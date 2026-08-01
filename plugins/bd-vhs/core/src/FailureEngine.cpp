#include "FailureEngine.h"

namespace bdvhs
{

namespace
{
    constexpr float kBaseRateHz = 0.02f;   // the occasional blemish even at zero
    constexpr float kMaxExtraRateHz = 6.0f;

    // Relative likelihood of each kind of malfunction.
    constexpr float kWeightDropout = 0.45f;
    constexpr float kWeightSnag    = 0.30f;
    constexpr float kWeightWrinkle = 0.20f;
    // Crackle takes the remainder, 0.05.

    constexpr float kWrinkleOpenHz   = 12000.0f;
    constexpr float kWrinkleClosedHz =  1200.0f;
    constexpr float kWrinkleAmHz     =    30.0f;
    constexpr float kWrinkleAmDepth  =     0.15f;
}

void FailureEngine::prepare (double sampleRate, uint32_t seed, int channelIndex)
{
    sr = sampleRate;
    rng.setSeed (seed ^ (0x85EBCA6Bu * static_cast<uint32_t> (channelIndex + 1)));

    wrinkleAm.prepare (sr, kWrinkleAmHz, 0.0f);

    // Snag displacement washes out over roughly half a second, so the tape
    // path drifts back to its nominal gap instead of accumulating offset
    // forever.
    snagDecay = std::exp (-1.0f / (0.5f * static_cast<float> (sr)));

    reset();
}

void FailureEngine::reset()
{
    for (auto& e : events)
        e = Event {};

    snagOffset = 0.0f;
    eventsStarted = 0;
    scheduleNext();
}

void FailureEngine::setAmount (float failure01) noexcept
{
    amount = clampf (failure01, 0.0f, 1.0f);
    rateHz = kBaseRateHz + amount * amount * kMaxExtraRateHz;
}

void FailureEngine::scheduleNext() noexcept
{
    // Exponentially distributed gap: the inter-arrival time of a Poisson
    // process with rate `rateHz`.
    const float u = rng.nextPositive();
    const float gapSeconds = -std::log (u) / std::fmax (rateHz, 1.0e-4f);
    samplesUntilNext = gapSeconds * static_cast<float> (sr);
}

void FailureEngine::startEvent (Event& e) noexcept
{
    const float roll = rng.nextFloat();

    float durationSec = 0.1f;

    if (roll < kWeightDropout)
    {
        e.type = EventType::Dropout;

        const float attackSec  = rng.nextRange (0.003f, 0.015f);
        const float holdSec    = rng.nextRange (0.010f, 0.120f);
        const float releaseSec = rng.nextRange (0.020f, 0.200f);
        durationSec = attackSec + holdSec + releaseSec;

        e.attackFrac = attackSec / durationSec;
        e.holdFrac   = (attackSec + holdSec) / durationSec;

        // Deeper dropouts become likely as the knob comes up.
        const float depthDb = rng.nextRange (-6.0f, -6.0f - 54.0f * amount);
        e.depthGain = dbToGain (depthDb);
    }
    else if (roll < kWeightDropout + kWeightSnag)
    {
        e.type = EventType::Snag;

        const float rampInSec  = 0.005f;
        const float holdSec    = rng.nextRange (0.020f, 0.090f);
        const float rampOutSec = 0.015f;
        durationSec = rampInSec + holdSec + rampOutSec;

        e.attackFrac = rampInSec / durationSec;
        e.holdFrac   = (rampInSec + holdSec) / durationSec;

        // Mostly the tape drags; occasionally it lurches forward.
        e.rate = (rng.nextFloat() < 0.75f) ? rng.nextRange (0.75f, 0.95f)
                                           : rng.nextRange (1.10f, 1.35f);
        e.depthGain = dbToGain (-3.0f);   // a snag is audible as a level dip too
    }
    else if (roll < kWeightDropout + kWeightSnag + kWeightWrinkle)
    {
        e.type = EventType::Wrinkle;
        durationSec = rng.nextRange (0.060f, 0.250f);
    }
    else
    {
        e.type = EventType::Crackle;
        e.cracklesLeft = 1 + static_cast<int> (rng.nextFloat() * 4.0f);
        e.crackleFreq  = rng.nextRange (1500.0f, 2800.0f);
        e.crackleDecay = std::exp (-1.0f / (0.002f * static_cast<float> (sr)));
        e.crackleTimer = 0.0f;
        e.cracklePhase = 0.0f;
        e.crackleEnv   = 0.0f;
        durationSec = 0.030f;
    }

    e.phase = 0.0f;
    e.phaseInc = 1.0f / (durationSec * static_cast<float> (sr));
    e.active = true;
    ++eventsStarted;
}

void FailureEngine::process (FailureOutput& out) noexcept
{
    out.gain = 1.0f;
    out.lpCutoffHz = 20000.0f;
    out.crackle = 0.0f;
    out.snagActive = false;

    samplesUntilNext -= 1.0f;
    if (samplesUntilNext <= 0.0f)
    {
        for (auto& e : events)
        {
            if (! e.active)
            {
                startEvent (e);
                break;
            }
        }
        scheduleNext();
    }

    float snagRateDeviation = 0.0f;

    for (auto& e : events)
    {
        if (! e.active)
            continue;

        e.phase += e.phaseInc;
        if (e.phase >= 1.0f)
        {
            e.active = false;
            continue;
        }

        switch (e.type)
        {
            case EventType::Dropout:
            case EventType::Snag:
            {
                // Raised-cosine attack and release; a linear ramp here would
                // click at the corners.
                float env;
                if (e.phase < e.attackFrac)
                    env = raisedCosine (e.phase / e.attackFrac);
                else if (e.phase < e.holdFrac)
                    env = 1.0f;
                else
                    env = raisedCosine (1.0f - (e.phase - e.holdFrac) / (1.0f - e.holdFrac));

                out.gain *= lerp (1.0f, e.depthGain, env);

                if (e.type == EventType::Snag)
                {
                    snagRateDeviation += (1.0f - e.rate) * env;
                    out.snagActive = true;
                }
                break;
            }

            case EventType::Wrinkle:
            {
                // Cutoff dives and recovers; the sweep is what makes it read as
                // a physical crease passing the head rather than as an EQ move.
                const float dip = std::sin (kPi * e.phase);
                out.lpCutoffHz = std::fmin (out.lpCutoffHz,
                                            lerpLog (kWrinkleOpenHz, kWrinkleClosedHz, dip));
                out.gain *= 1.0f + kWrinkleAmDepth * dip * wrinkleAm.sine();
                break;
            }

            case EventType::Crackle:
            {
                e.crackleTimer -= 1.0f;
                if (e.crackleTimer <= 0.0f && e.cracklesLeft > 0)
                {
                    --e.cracklesLeft;
                    e.crackleEnv = rng.nextRange (0.15f, 0.5f) * (0.3f + 0.7f * amount);
                    e.cracklePhase = 0.0f;
                    e.crackleTimer = rng.nextRange (0.002f, 0.012f) * static_cast<float> (sr);
                }

                if (e.crackleEnv > 1.0e-6f)
                {
                    e.cracklePhase += kTwoPi * e.crackleFreq / static_cast<float> (sr);
                    if (e.cracklePhase > kTwoPi)
                        e.cracklePhase -= kTwoPi;

                    out.crackle += e.crackleEnv * std::sin (e.cracklePhase);
                    e.crackleEnv *= e.crackleDecay;
                }
                break;
            }
        }
    }

    wrinkleAm.tick();

    snagOffset = flushDenormal (snagOffset * snagDecay + snagRateDeviation);
    out.delayOffsetSamples = snagOffset;
}

} // namespace bdvhs
