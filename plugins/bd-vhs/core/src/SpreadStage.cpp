#include "SpreadStage.h"

namespace bdvhs
{

void SpreadStage::prepare (double sampleRate, uint32_t seed)
{
    sr = sampleRate;
    rng.setSeed (seed ^ 0x1B873593u);

    baseDelaySamples = kBaseDelayMs * 0.001f * static_cast<float> (sr);
    maxOffsetSamples = kMaxOffsetMs * 0.001f * static_cast<float> (sr);

    const int maxSamples = static_cast<int> (baseDelaySamples + maxOffsetSamples) + 8;
    delayL.prepare (maxSamples);
    delayR.prepare (maxSamples);

    wander.prepare (sr, 0.13f, rng);
    imbalance.prepare (sr, 0.19f, rng);

    amount.setTimeMs (200.0f, sr);
    amount.reset (0.0f);

    reset();
}

void SpreadStage::reset()
{
    delayL.reset();
    delayR.reset();
    wander.reset (rng);
    imbalance.reset (rng);
    amount.reset (amount.getTarget());
}

void SpreadStage::setAmount (float failure01, bool enabled) noexcept
{
    active = enabled;
    amount.setTarget (enabled ? clampf (failure01, 0.0f, 1.0f) : 0.0f);
}

void SpreadStage::process (float& left, float& right) noexcept
{
    const float a = amount.next();

    // Keep the lines primed even while bypassed, so switching SPREAD on does
    // not read a millisecond of stale silence.
    delayL.write (left);
    delayR.write (right);

    if (! active && a <= 0.0f)
        return;

    const float w = wander.next (rng);
    const float b = imbalance.next (rng);

    const float l = delayL.read (baseDelaySamples);
    const float r = delayR.read (baseDelaySamples + a * maxOffsetSamples * w);

    float mid  = 0.5f * (l + r);
    float side = 0.5f * (l - r);

    side *= 1.0f + a * 0.8f * b;
    mid  *= 1.0f - a * 0.2f * std::fabs (b);

    // However far the image wanders, keep enough mid that a mono sum survives.
    const float sideLimit = 2.0f * std::fabs (mid);
    side = clampf (side, -sideLimit, sideLimit);

    left  = mid + side;
    right = mid - side;
}

} // namespace bdvhs
