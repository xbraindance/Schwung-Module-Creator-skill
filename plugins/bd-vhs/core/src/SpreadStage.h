#pragma once

#include "dsp/DelayLine.h"
#include "dsp/Random.h"
#include "dsp/Smoothing.h"

namespace bdvhs
{

/**
    SPREAD: lets FAILURE destabilise the stereo image as well as the signal.

    A slowly wandering inter-channel delay offset plus mid/side imbalance. The
    side component is clamped against the mid so that however badly the image
    misbehaves, a mono sum can never collapse to silence.

    When disabled this is a bit-exact pass-through -- it does not even run its
    delay lines -- so the default path costs nothing.
*/
class SpreadStage
{
public:
    void prepare (double sampleRate, uint32_t seed);
    void reset();

    /** Control-rate. */
    void setAmount (float failure01, bool enabled) noexcept;

    /** Advances one sample, in place. */
    void process (float& left, float& right) noexcept;

private:
    static constexpr float kBaseDelayMs = 1.0f;
    static constexpr float kMaxOffsetMs = 0.8f;

    double sr = 44100.0;
    bool active = false;

    DelayLine delayL, delayR;
    SmoothRandom wander, imbalance;
    Xoshiro128 rng;

    Smoother amount;
    float baseDelaySamples = 0.0f;
    float maxOffsetSamples = 0.0f;
};

} // namespace bdvhs
