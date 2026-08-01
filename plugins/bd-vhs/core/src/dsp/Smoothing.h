#pragma once

#include "Common.h"

namespace bdvhs
{

/**
    Per-sample one-pole smoother.

    Deliberately per-sample rather than per-control-block: a control-block
    staircase on a gain is audible as zipper noise under fast automation, and a
    one-pole is cheap enough that there is no reason to accept that. Because it
    depends only on the sample index and not on where block boundaries fall,
    it is also block-size invariant.
*/
class Smoother
{
public:
    void setTimeMs (float ms, double sampleRate) noexcept
    {
        const float samples = std::fmax (1.0f, static_cast<float> (sampleRate) * ms * 0.001f);
        coeff = 1.0f - std::exp (-1.0f / samples);
    }

    void reset (float value) noexcept { current = target = value; }

    void setTarget (float value) noexcept { target = value; }

    float next() noexcept
    {
        current += coeff * (target - current);
        if (std::fabs (target - current) < 1.0e-9f)
            current = target;
        return current;
    }

    float value() const noexcept { return current; }
    float getTarget() const noexcept { return target; }

private:
    float coeff = 1.0f, current = 0.0f, target = 0.0f;
};

/**
    Equal-power crossfade helper for the discrete switches (DRY level, AUX
    filter bypass, bypass) where a linear fade would dip in the middle.
*/
inline void equalPowerGains (float t, float& gainA, float& gainB) noexcept
{
    const float clamped = clampf (t, 0.0f, 1.0f);
    gainA = std::cos (clamped * kPi * 0.5f);
    gainB = std::sin (clamped * kPi * 0.5f);
}

} // namespace bdvhs
