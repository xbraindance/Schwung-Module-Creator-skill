#pragma once

#include <cmath>
#include <cstdint>

namespace bdvhs
{

constexpr float kPi     = 3.14159265358979323846f;
constexpr float kTwoPi  = 6.28318530717958647693f;

/** Below this magnitude a recursive filter state is worthless signal and
    expensive to keep (denormals). Flushing satisfies the core's contract that
    every sample is either exactly zero or at least 1e-25 in magnitude. */
constexpr float kDenormalFloor = 1.0e-25f;

inline float flushDenormal (float x) noexcept
{
    return (std::fabs (x) < kDenormalFloor) ? 0.0f : x;
}

inline float clampf (float x, float lo, float hi) noexcept
{
    return x < lo ? lo : (x > hi ? hi : x);
}

inline float dbToGain (float db) noexcept
{
    return std::pow (10.0f, db * 0.05f);
}

inline float gainToDb (float g) noexcept
{
    return 20.0f * std::log10 (std::fmax (g, 1.0e-12f));
}

inline float lerp (float a, float b, float t) noexcept
{
    return a + (b - a) * t;
}

/** Interpolates a frequency or Q geometrically. Interpolating filter *design*
    parameters this way keeps every intermediate point a valid filter design,
    which linear interpolation of biquad coefficients does not. */
inline float lerpLog (float a, float b, float t) noexcept
{
    return std::exp (lerp (std::log (a), std::log (b), t));
}

/** Hermite smoothstep between two thresholds. */
inline float smoothstep (float x, float edge0, float edge1) noexcept
{
    const float t = clampf ((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/** Raised cosine ramp, 0 -> 1 over t in [0, 1]. C1-continuous at both ends,
    which is why failure envelopes built from it do not click. */
inline float raisedCosine (float t) noexcept
{
    return 0.5f - 0.5f * std::cos (kPi * clampf (t, 0.0f, 1.0f));
}

inline int nextPowerOfTwo (int n) noexcept
{
    int p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

} // namespace bdvhs
