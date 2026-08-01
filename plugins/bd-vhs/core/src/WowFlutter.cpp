#include "WowFlutter.h"

namespace bdvhs
{

namespace
{
    // Incommensurate rates: no common divisor, so the combined motion never
    // settles into an audible loop.
    constexpr float kWowRates[3]     = { 0.437f, 0.550f, 0.693f };
    constexpr float kWowAmps[3]      = { 0.55f,  1.00f,  0.35f  };
    constexpr float kWowDriftRate    = 0.80f;
    constexpr float kWowDriftAmp     = 0.60f;
    constexpr float kWowNorm         = 1.0f / (0.55f + 1.00f + 0.35f + 0.60f);

    constexpr float kFlutterRates[2] = { 8.30f, 11.70f };
    constexpr float kFlutterAmps[2]  = { 1.00f,  0.70f };
    constexpr float kFlutterJitRate  = 11.0f;
    constexpr float kFlutterJitAmp   = 0.50f;
    constexpr float kScrapeRate      = 42.0f;
    constexpr float kScrapeAmp       = 0.12f;
    constexpr float kFlutterNorm     = 1.0f / (1.00f + 0.70f + 0.50f + 0.12f);

    // Peak amplitude modulation depth at full flutter: about +/- 2.2 dB.
    constexpr float kAmDepth = 0.25f;

    /** Makes the amplitude wobble spiky rather than sinusoidal, which is what
        reads as twitchy rather than as tremolo. */
    inline float spikeShape (float x) noexcept
    {
        return (x < 0.0f ? -1.0f : 1.0f) * std::pow (std::fabs (x), 0.7f);
    }
}

void WowFlutter::prepare (double sampleRate, uint32_t seed, int channelIndex)
{
    sr = sampleRate;
    rng.setSeed (seed ^ (0x9E3779B9u * static_cast<uint32_t> (channelIndex + 1)));

    // The right channel runs its oscillators slightly faster and from different
    // phases, so the two channels drift apart over minutes rather than tracking
    // each other. Core crossfades the two modulation signals afterwards to keep
    // the result mono-compatible.
    const float rateScale  = (channelIndex == 0) ? 1.0f : 1.031f;
    const float phaseSeed  = (channelIndex == 0) ? 0.0f : 0.37f;

    for (int i = 0; i < 3; ++i)
        wowOsc[i].prepare (sr, kWowRates[i] * rateScale,
                           phaseSeed + 0.61f * static_cast<float> (i));

    for (int i = 0; i < 2; ++i)
        flutterOsc[i].prepare (sr, kFlutterRates[i] * rateScale,
                               phaseSeed + 0.43f * static_cast<float> (i));

    scrapeOsc.prepare (sr, kScrapeRate * rateScale, phaseSeed);

    wowDrift.prepare (sr, kWowDriftRate, rng);
    flutterJitter.prepare (sr, kFlutterJitRate, rng);

    wowScale     = kMaxWowMs     * 0.001f * static_cast<float> (sr);
    flutterScale = kMaxFlutterMs * 0.001f * static_cast<float> (sr);

    wowDepth.setTimeMs (20.0f, sr);
    flutterDepth.setTimeMs (20.0f, sr);
    wowDepth.reset (0.0f);
    flutterDepth.reset (0.0f);
}

void WowFlutter::reset()
{
    wowDrift.reset (rng);
    flutterJitter.reset (rng);
    wowDepth.reset (wowDepth.getTarget());
    flutterDepth.reset (flutterDepth.getTarget());
}

void WowFlutter::setDepth (float wow01, float flutter01) noexcept
{
    wowDepth.setTarget (clampf (wow01, 0.0f, 1.0f));
    flutterDepth.setTarget (clampf (flutter01, 0.0f, 1.0f));
}

void WowFlutter::process (float& delayDeviationSamples, float& amplitudeGain) noexcept
{
    float wowSum = kWowDriftAmp * wowDrift.next (rng);
    for (int i = 0; i < 3; ++i)
    {
        wowSum += kWowAmps[i] * wowOsc[i].sine();
        wowOsc[i].tick();
    }
    wowSum *= kWowNorm;

    float flutterSum = kFlutterJitAmp * flutterJitter.next (rng);
    float flutterQuad = 0.0f;
    for (int i = 0; i < 2; ++i)
    {
        flutterSum  += kFlutterAmps[i] * flutterOsc[i].sine();
        flutterQuad += kFlutterAmps[i] * flutterOsc[i].cosine();
        flutterOsc[i].tick();
    }
    flutterSum  += kScrapeAmp * scrapeOsc.sine();
    flutterQuad += kScrapeAmp * scrapeOsc.cosine();
    scrapeOsc.tick();

    flutterSum  *= kFlutterNorm;
    flutterQuad *= kFlutterNorm;

    const float wd = wowDepth.next();
    const float fd = flutterDepth.next();

    delayDeviationSamples = wd * wowScale * wowSum + fd * flutterScale * flutterSum;

    // Amplitude modulation scales with the square of flutter, so low settings
    // are pure pitch wobble and the amplitude character only arrives at the top
    // of the knob.
    amplitudeGain = 1.0f + kAmDepth * fd * fd * spikeShape (flutterQuad);
}

} // namespace bdvhs
