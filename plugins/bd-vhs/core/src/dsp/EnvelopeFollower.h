#pragma once

#include "Common.h"

namespace bdvhs
{

/**
    Peak follower with independent attack and release. Drives the hiss's
    "noise response" modes, which need to know whether anything is playing.
*/
class EnvelopeFollower
{
public:
    void prepare (double sampleRate, float attackMs, float releaseMs) noexcept
    {
        const float sr = static_cast<float> (sampleRate);
        attackCoeff  = coeffFor (attackMs,  sr);
        releaseCoeff = coeffFor (releaseMs, sr);
        env = 0.0f;
    }

    void reset() noexcept { env = 0.0f; }

    float process (float x) noexcept
    {
        const float rectified = std::fabs (x);
        const float c = (rectified > env) ? attackCoeff : releaseCoeff;
        env = flushDenormal (env + c * (rectified - env));
        return env;
    }

    float current() const noexcept { return env; }

private:
    static float coeffFor (float ms, float sampleRate) noexcept
    {
        const float samples = std::fmax (1.0f, sampleRate * ms * 0.001f);
        return 1.0f - std::exp (-1.0f / samples);
    }

    float attackCoeff = 1.0f, releaseCoeff = 1.0f, env = 0.0f;
};

} // namespace bdvhs
