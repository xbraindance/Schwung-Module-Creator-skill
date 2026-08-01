#pragma once

#include "Common.h"

namespace bdvhs
{

/**
    xoshiro128+ -- small, fast, and above all reproducible. The core never calls
    rand() or std::random_device, so a given seed always produces exactly the
    same noise and the same sequence of failure events.
*/
class Xoshiro128
{
public:
    explicit Xoshiro128 (uint32_t seed = 0x5EEDBD42u) { setSeed (seed); }

    void setSeed (uint32_t seed) noexcept
    {
        // SplitMix32 the seed out into the four state words so that adjacent
        // seeds (e.g. seed ^ laneIndex) produce well-separated streams.
        uint32_t z = seed + 0x9E3779B9u;
        for (int i = 0; i < 4; ++i)
        {
            uint32_t x = z;
            z += 0x9E3779B9u;
            x ^= x >> 16; x *= 0x85EBCA6Bu;
            x ^= x >> 13; x *= 0xC2B2AE35u;
            x ^= x >> 16;
            s[i] = x;
        }
        if ((s[0] | s[1] | s[2] | s[3]) == 0u)
            s[0] = 1u;
    }

    uint32_t nextUInt() noexcept
    {
        const uint32_t result = s[0] + s[3];
        const uint32_t t = s[1] << 9;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = (s[3] << 11) | (s[3] >> 21);

        return result;
    }

    /** Uniform in [0, 1). */
    float nextFloat() noexcept
    {
        return static_cast<float> (nextUInt() >> 8) * (1.0f / 16777216.0f);
    }

    /** Uniform in [-1, 1). */
    float nextBipolar() noexcept
    {
        return nextFloat() * 2.0f - 1.0f;
    }

    /** Uniform in [lo, hi). */
    float nextRange (float lo, float hi) noexcept
    {
        return lo + (hi - lo) * nextFloat();
    }

    /** Uniform in (0, 1] -- safe to take the logarithm of, which the Poisson
        event scheduler needs. */
    float nextPositive() noexcept
    {
        return 1.0f - nextFloat();
    }

private:
    uint32_t s[4] {};
};

/**
    Smoothly interpolated random motion, in roughly [-1, 1].

    A new random target is drawn every 1/rateHz seconds and reached along a
    raised-cosine path, which has zero derivative at both ends -- so the output
    is C1-continuous and never corners. This replaces the more obvious
    "white noise through cascaded one-poles": those need enormous, sample-rate
    dependent make-up gain to get back to a usable amplitude, which is fragile.
    This does not.
*/
class SmoothRandom
{
public:
    void prepare (double sampleRate, float rateHz, Xoshiro128& rng) noexcept
    {
        phaseInc = rateHz / static_cast<float> (sampleRate);
        phase = 0.0f;
        prev = rng.nextBipolar();
        target = rng.nextBipolar();
    }

    void reset (Xoshiro128& rng) noexcept
    {
        phase = 0.0f;
        prev = rng.nextBipolar();
        target = rng.nextBipolar();
    }

    float next (Xoshiro128& rng) noexcept
    {
        phase += phaseInc;
        while (phase >= 1.0f)
        {
            phase -= 1.0f;
            prev = target;
            target = rng.nextBipolar();
        }
        return lerp (prev, target, raisedCosine (phase));
    }

private:
    float phaseInc = 0.0f, phase = 0.0f, prev = 0.0f, target = 0.0f;
};

/**
    Quadrature sine oscillator built from a 2x2 rotation. Two outputs 90 degrees
    apart for the price of one, which the flutter stage uses to derive an
    amplitude-modulation signal that is related to but decorrelated from the
    pitch modulation.
*/
class QuadOsc
{
public:
    void prepare (double sampleRate, float freqHz, float phase01) noexcept
    {
        const float w = kTwoPi * freqHz / static_cast<float> (sampleRate);
        cosW = std::cos (w);
        sinW = std::sin (w);

        const float p = kTwoPi * phase01;
        x = std::cos (p);
        y = std::sin (p);
        counter = 0;
    }

    void tick() noexcept
    {
        const float nx = x * cosW - y * sinW;
        const float ny = x * sinW + y * cosW;
        x = nx;
        y = ny;

        // Rotation matrices drift off the unit circle in float. Renormalising
        // every few thousand samples costs nothing and keeps amplitude exact.
        if (++counter >= 4096)
        {
            counter = 0;
            const float mag = std::sqrt (x * x + y * y);
            if (mag > 1.0e-6f)
            {
                x /= mag;
                y /= mag;
            }
            else
            {
                x = 1.0f;
                y = 0.0f;
            }
        }
    }

    float sine()   const noexcept { return y; }
    float cosine() const noexcept { return x; }

private:
    float cosW = 1.0f, sinW = 0.0f, x = 1.0f, y = 0.0f;
    int counter = 0;
};

/**
    Paul Kellett's 3-pole pink noise approximation. Close enough to -3 dB/octave
    across the audible band, and cheap.
*/
class PinkNoise
{
public:
    void reset() noexcept { b0 = b1 = b2 = 0.0f; }

    float next (Xoshiro128& rng) noexcept
    {
        const float white = rng.nextBipolar();
        b0 = flushDenormal (0.99765f * b0 + white * 0.0990460f);
        b1 = flushDenormal (0.96300f * b1 + white * 0.2965164f);
        b2 = flushDenormal (0.57000f * b2 + white * 1.0526913f);
        return (b0 + b1 + b2 + white * 0.1848f) * 0.25f;
    }

private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
};

} // namespace bdvhs
