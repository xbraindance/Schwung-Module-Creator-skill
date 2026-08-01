#include <bdvhs/BdVhsCore.h>

#include "AuxPerformance.h"
#include "FailureEngine.h"
#include "ModelEq.h"
#include "ModelProfiles.h"
#include "NoiseEngine.h"
#include "Saturator.h"
#include "SpreadStage.h"
#include "WowFlutter.h"

#include "dsp/DelayLine.h"
#include "dsp/EnvelopeFollower.h"
#include "dsp/Smoothing.h"
#include "dsp/Svf.h"

#include <algorithm>

namespace bdvhs
{

namespace
{
    constexpr int kMaxChannels = 2;

    /** Wet L/R occupy lanes 0-1; when DRY TYPE is Processed the dry blend rides
        lanes 2-3 through the same machine, minus wow and flutter. */
    constexpr int kMaxLanes = 4;

    constexpr uint32_t kDefaultSeed = 0x5EEDBD42u;

    /** How much of the left channel's transport wobble bleeds into the right.
        Fully independent modulation sounds impressive in stereo and collapses
        badly in mono; this is the compromise. */
    constexpr float kModLink = 0.65f;

    constexpr float kTapeBufferSeconds = 2.0f;

    constexpr float kSlewNormal = 0.25f;   // samples of delay change per sample
    constexpr float kSlewSnag   = 1.00f;

    constexpr float kDryGainSmallDb = -12.0f;

    /** Only engages above full scale, and asymptotes to this, so a pathological
        failure event can never hand the host something unbounded. */
    inline float safetyClip (float x) noexcept
    {
        const float a = std::fabs (x);
        if (a <= 1.0f)
            return x;
        const float sign = (x < 0.0f) ? -1.0f : 1.0f;
        return sign * (1.0f + 0.4f * std::tanh ((a - 1.0f) / 0.4f));
    }

    inline float dryLevelGain (DryLevel level) noexcept
    {
        switch (level)
        {
            case DryLevel::Small: return dbToGain (kDryGainSmallDb);
            case DryLevel::Unity: return 1.0f;
            case DryLevel::None:
            default:              return 0.0f;
        }
    }
}

// ============================================================================

struct Core::Impl
{
    double sr = 44100.0;
    int maxBlock = 512;
    int numChannels = 2;
    uint32_t seed = kDefaultSeed;

    Params params;

    // --- per channel ---------------------------------------------------
    WowFlutter    wowFlutter[kMaxChannels];
    FailureEngine failure[kMaxChannels];
    NoiseEngine   noise[kMaxChannels];
    DelayLine     tapeDelay[kMaxChannels];
    DelayLine     dryAlign[kMaxChannels];
    DcBlocker     inputDc[kMaxChannels];
    float         currentMod[kMaxChannels] {};

    // --- per lane ------------------------------------------------------
    ModelEq   modelEq[kMaxLanes];
    Saturator saturator[kMaxLanes];
    OnePoleLp wrinkleLp[kMaxLanes];
    float     lastWrinkleHz[kMaxLanes] { 20000.0f, 20000.0f, 20000.0f, 20000.0f };

    SpreadStage spread;
    AuxPerformance aux;
    EnvelopeFollower inputFollower;

    Smoother inputGain, volumeGain, mixGain, dryBlend, bypassFade;

    MachineProfile profile = kProfiles[0];
    float noiseResponseGain = 1.0f;
    float auxOffsetSamples = 0.0f;
    float auxRestartDecay = 0.0f;
    float lastRamp = 0.0f;

    float baseDelaySamples = 0.0f;
    float maxAuxOffset = 0.0f;
    int   alignSamples = Core::latencySamples();

    int controlCountdown = 0;

    void prepare (double sampleRate, int maxBlockSamples, int channels);
    void reset();
    void updateControl();
    void renderSample (float* const* io, int frame, int channels);
};

void Core::Impl::prepare (double sampleRate, int maxBlockSamples, int channels)
{
    sr = sampleRate;
    maxBlock = std::max (1, maxBlockSamples);
    numChannels = std::min (kMaxChannels, std::max (1, channels));

    baseDelaySamples = WowFlutter::kBaseDelayMs * 0.001f * static_cast<float> (sr);
    alignSamples = Core::latencySamples();

    const int tapeSamples = static_cast<int> (kTapeBufferSeconds * static_cast<float> (sr));

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        wowFlutter[ch].prepare (sr, seed, ch);
        failure[ch].prepare (sr, seed, ch);
        noise[ch].prepare (sr, seed, ch);

        tapeDelay[ch].prepare (tapeSamples);
        dryAlign[ch].prepare (alignSamples + 8);

        inputDc[ch].setCutoff (12.0f, static_cast<float> (sr));
    }

    // Leave room for the tape-stop wind-down without ever running the read
    // pointer past the write pointer.
    maxAuxOffset = static_cast<float> (tapeDelay[0].capacity()) - baseDelaySamples
                   - WowFlutter::kMaxWowMs * 0.001f * static_cast<float> (sr) - 64.0f;
    maxAuxOffset = std::fmax (0.0f, maxAuxOffset);

    for (int lane = 0; lane < kMaxLanes; ++lane)
    {
        modelEq[lane].prepare (sr);
        saturator[lane].prepare (sr);
        wrinkleLp[lane].setCutoff (20000.0f, static_cast<float> (sr));
    }

    spread.prepare (sr, seed);
    aux.prepare (sr);
    inputFollower.prepare (sr, 5.0f, 400.0f);

    inputGain.setTimeMs (20.0f, sr);
    volumeGain.setTimeMs (20.0f, sr);
    mixGain.setTimeMs (20.0f, sr);
    dryBlend.setTimeMs (8.0f, sr);
    bypassFade.setTimeMs (10.0f, sr);

    auxRestartDecay = std::exp (-1.0f / (0.25f * static_cast<float> (sr)));

    reset();

    inputGain.reset (dbToGain (params.inputGainDb));
    volumeGain.reset (dbToGain (params.volumeDb));
    mixGain.reset (params.mix * 0.01f);
    dryBlend.reset (dryLevelGain (params.dryLevel));
    bypassFade.reset (params.bypass ? 1.0f : 0.0f);

    updateControl();
}

void Core::Impl::reset()
{
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        wowFlutter[ch].reset();
        failure[ch].reset();
        noise[ch].reset();
        tapeDelay[ch].reset();
        dryAlign[ch].reset();
        inputDc[ch].reset();
        currentMod[ch] = 0.0f;
    }

    for (int lane = 0; lane < kMaxLanes; ++lane)
    {
        modelEq[lane].reset();
        saturator[lane].reset();
        wrinkleLp[lane].reset (0.0f);
        wrinkleLp[lane].setCutoff (20000.0f, static_cast<float> (sr));
        lastWrinkleHz[lane] = 20000.0f;
    }

    spread.reset();
    aux.reset();
    inputFollower.reset();

    auxOffsetSamples = 0.0f;
    lastRamp = 0.0f;
    controlCountdown = 0;
}

void Core::Impl::updateControl()
{
    // Ramp and bounce pull the three motion controls toward maximum.
    const float ramp = lastRamp;

    const float wow01     = lerp (clampf (params.wow     * 0.01f, 0.0f, 1.0f), 1.0f, ramp);
    const float flutter01 = lerp (clampf (params.flutter * 0.01f, 0.0f, 1.0f), 1.0f, ramp);
    float failure01       = lerp (clampf (params.failure * 0.01f, 0.0f, 1.0f), 1.0f, ramp);

    profile = interpolateProfile (clampf (params.model * 0.01f, 0.0f, 1.0f), params.modelSnap);

    const float saturate01 = clampf (params.saturate * 0.01f, 0.0f, 1.0f);

    for (int lane = 0; lane < kMaxLanes; ++lane)
    {
        modelEq[lane].setProfile (profile);
        saturator[lane].setDrive (saturate01, profile.satBias);
    }

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        wowFlutter[ch].setDepth (wow01, flutter01);
        failure[ch].setAmount (failure01);
        noise[ch].setProfile (profile, params.noise);
    }

    spread.setAmount (failure01, params.spread);
    aux.update (params);

    // Noise response: does the hiss sit there, arrive with the music, or fill
    // the gaps?
    const float envDb = gainToDb (inputFollower.current());
    switch (params.noiseResponse)
    {
        case NoiseResponse::Gated:
            noiseResponseGain = smoothstep (envDb, -60.0f, -30.0f);
            break;
        case NoiseResponse::Ducked:
            noiseResponseGain = 1.0f - 0.7f * smoothstep (envDb, -60.0f, -20.0f);
            break;
        case NoiseResponse::Static:
        default:
            noiseResponseGain = 1.0f;
            break;
    }

    inputGain.setTarget (dbToGain (clampf (params.inputGainDb, -24.0f, 24.0f)));
    volumeGain.setTarget (dbToGain (clampf (params.volumeDb, -60.0f, 12.0f)));
    mixGain.setTarget (clampf (params.mix * 0.01f, 0.0f, 1.0f));
    dryBlend.setTarget (dryLevelGain (params.dryLevel));
    bypassFade.setTarget (params.bypass ? 1.0f : 0.0f);
}

void Core::Impl::renderSample (float* const* io, int frame, int channels)
{
    const bool stereo = (channels > 1);

    float raw[kMaxChannels] {};
    raw[0] = io[0][frame];
    raw[1] = stereo ? io[1][frame] : raw[0];

    // ---- dry alignment ---------------------------------------------------
    // Delayed to match the saturator's oversampling latency, so that the dry
    // blend and the wet path leave the plugin in step.
    float dryRaw[kMaxChannels] {};
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        dryAlign[ch].write (raw[ch]);
        dryRaw[ch] = dryAlign[ch].readInt (alignSamples);
    }

    inputFollower.process (0.5f * (raw[0] + raw[1]));

    AuxState auxState;
    aux.process (auxState);
    lastRamp = auxState.ramp;

    // Tape stop integrates a rate deviation into the read position, and
    // unwinds it on release -- which is what produces the speed-up as the
    // transport catches back up.
    if (auxState.rateDeviation > 0.0f)
        auxOffsetSamples = std::fmin (maxAuxOffset, auxOffsetSamples + auxState.rateDeviation);
    else
        auxOffsetSamples = flushDenormal (auxOffsetSamples * auxRestartDecay);

    // ---- input stage -----------------------------------------------------
    const float ig = inputGain.next();
    float x[kMaxChannels] {};
    for (int ch = 0; ch < kMaxChannels; ++ch)
        x[ch] = inputDc[ch].process (raw[ch]) * ig;

    // ---- transport instability ------------------------------------------
    float dev[kMaxChannels] {}, am[kMaxChannels] {};
    for (int ch = 0; ch < kMaxChannels; ++ch)
        wowFlutter[ch].process (dev[ch], am[ch]);

    // Bleed the channels together so the effect stays mono-compatible.
    const float devR = lerp (dev[1], dev[0], kModLink);
    dev[1] = devR;

    FailureOutput fail[kMaxChannels];
    for (int ch = 0; ch < kMaxChannels; ++ch)
        failure[ch].process (fail[ch]);

    // With SPREAD off both channels share one stream of malfunctions, so a
    // dropout is a dropout rather than a lurch in the stereo image.
    if (! params.spread)
        fail[1] = fail[0];

    float wet[kMaxChannels] {};
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        tapeDelay[ch].write (x[ch]);

        const float target = dev[ch] + fail[ch].delayOffsetSamples;
        const float maxStep = fail[ch].snagActive ? kSlewSnag : kSlewNormal;
        currentMod[ch] += clampf (target - currentMod[ch], -maxStep, maxStep);

        const float delaySamples = baseDelaySamples + currentMod[ch] + auxOffsetSamples;
        wet[ch] = tapeDelay[ch].read (delaySamples) * am[ch];
    }

    // ---- lanes -----------------------------------------------------------
    // Lane 0/1 are the wet channels; 2/3 carry the dry blend through the same
    // machine when DRY TYPE is Processed. One code path, and the dry lanes cost
    // nothing at all when they are not in use.
    static constexpr int laneChannel[kMaxLanes] = { 0, 1, 0, 1 };
    static constexpr bool laneIsWet[kMaxLanes]  = { true, true, false, false };

    const bool processedDry = (params.dryType == DryType::Processed);

    float lane[kMaxLanes] {};
    bool laneActive[kMaxLanes] = { true, false, false, false };

    lane[0] = wet[0];
    if (stereo)
    {
        lane[1] = wet[1];
        laneActive[1] = true;
    }

    if (processedDry)
    {
        // Tapped after the tape delay, not before the machine: the dry blend
        // shares everything except wow and flutter.
        lane[2] = x[0];
        laneActive[2] = true;

        if (stereo)
        {
            lane[3] = x[1];
            laneActive[3] = true;
        }
    }

    float fbA = 0.0f, fbB = 0.0f;
    equalPowerGains (auxState.filterBypass, fbA, fbB);

    const float srf = static_cast<float> (sr);

    for (int i = 0; i < kMaxLanes; ++i)
    {
        if (! laneActive[i])
            continue;

        const int ch = laneChannel[i];
        float v = lane[i];

        // AUX = Filter lifts the bandwidth restriction while it is held.
        v = fbA * modelEq[i].process (v) + fbB * v;

        v = saturator[i].process (v);

        if (laneIsWet[i])
            v += noise[ch].process (noiseResponseGain) + fail[ch].crackle;

        // Recomputing the wrinkle cutoff costs a tan(); it is worth skipping
        // while nothing is creasing the tape, which is nearly all of the time.
        if (std::fabs (fail[ch].lpCutoffHz - lastWrinkleHz[i]) > 0.01f * lastWrinkleHz[i])
        {
            wrinkleLp[i].setCutoff (fail[ch].lpCutoffHz, srf);
            lastWrinkleHz[i] = fail[ch].lpCutoffHz;
        }
        v = wrinkleLp[i].process (v);

        v *= fail[ch].gain;

        lane[i] = v;
    }

    wet[0] = lane[0];
    wet[1] = stereo ? lane[1] : lane[0];

    float dryLane[kMaxChannels] {};
    dryLane[0] = processedDry ? lane[2] : dryRaw[0];
    dryLane[1] = processedDry ? (stereo ? lane[3] : lane[2]) : dryRaw[1];

    if (stereo)
        spread.process (wet[0], wet[1]);

    // ---- blend and output ------------------------------------------------
    const float dry = dryBlend.next();
    const float mix = mixGain.next();
    const float vol = volumeGain.next();
    const float byp = bypassFade.next();

    for (int ch = 0; ch < channels; ++ch)
    {
        const float machine = (wet[ch] + dry * dryLane[ch]) * auxState.levelGain;
        float y = lerp (dryRaw[ch], machine, mix) * vol;
        y = lerp (y, dryRaw[ch], byp);

        // Flushing here as well as inside the recursive states guarantees the
        // core's contract that every output sample is either exactly zero or
        // large enough not to be a denormal.
        io[ch][frame] = flushDenormal (safetyClip (y));
    }
}

// ============================================================================

Core::Core() : impl (new Impl) {}
Core::~Core() = default;

void Core::prepare (double sampleRate, int maxBlockSamples, int numChannels)
{
    impl->prepare (sampleRate, maxBlockSamples, numChannels);
}

void Core::reset()
{
    impl->reset();
}

void Core::setSeed (uint32_t seed)
{
    impl->seed = seed;
    // Reseeding has to rebuild the generators, so route it through prepare()
    // with the settings already in force.
    impl->prepare (impl->sr, impl->maxBlock, impl->numChannels);
}

void Core::setParams (const Params& p)
{
    impl->params = p;
}

void Core::process (float* const* io, int numChannels, int numSamples)
{
    const int channels = std::min (numChannels, kMaxChannels);
    if (channels <= 0 || numSamples <= 0)
        return;

    int frame = 0;
    while (frame < numSamples)
    {
        // The control grid is measured from the start of the stream, not from
        // the start of this block. That is what makes the output identical
        // however the host chooses to chop up the audio.
        if (impl->controlCountdown <= 0)
        {
            impl->updateControl();
            impl->controlCountdown = controlBlockSamples();
        }

        const int n = std::min (impl->controlCountdown, numSamples - frame);

        for (int i = 0; i < n; ++i)
            impl->renderSample (io, frame + i, channels);

        impl->controlCountdown -= n;
        frame += n;
    }
}

int Core::failureEventCount() const
{
    return impl->failure[0].eventCount() + impl->failure[1].eventCount();
}

} // namespace bdvhs
