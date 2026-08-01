#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include "TestMath.h"

namespace bdvhs::test
{

/** Deterministic PRNG for test signals, kept separate from the core's own so
    that changing one never perturbs the other. */
class TestRng
{
public:
    explicit TestRng (uint32_t seed = 12345u) : state (seed ? seed : 1u) {}

    uint32_t nextUInt()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    float nextBipolar()
    {
        return static_cast<float> (nextUInt() >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }

    float nextRange (float lo, float hi)
    {
        return lo + (hi - lo) * (static_cast<float> (nextUInt() >> 8) * (1.0f / 16777216.0f));
    }

private:
    uint32_t state;
};

inline std::vector<float> silence (size_t n)
{
    return std::vector<float> (n, 0.0f);
}

inline std::vector<float> sine (size_t n, float freqHz, double sampleRate, float amplitude = 0.5f)
{
    std::vector<float> out (n);
    const double w = 2.0 * kPiD * static_cast<double> (freqHz) / sampleRate;
    for (size_t i = 0; i < n; ++i)
        out[i] = amplitude * static_cast<float> (std::sin (w * static_cast<double> (i)));
    return out;
}

inline std::vector<float> impulse (size_t n, size_t at = 0, float amplitude = 1.0f)
{
    std::vector<float> out (n, 0.0f);
    if (at < n)
        out[at] = amplitude;
    return out;
}

inline std::vector<float> whiteNoise (size_t n, uint32_t seed = 7u, float amplitude = 0.3f)
{
    std::vector<float> out (n);
    TestRng rng (seed);
    for (size_t i = 0; i < n; ++i)
        out[i] = amplitude * rng.nextBipolar();
    return out;
}

inline std::vector<float> pinkNoise (size_t n, uint32_t seed = 11u, float amplitude = 0.3f)
{
    std::vector<float> out (n);
    TestRng rng (seed);
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;

    for (size_t i = 0; i < n; ++i)
    {
        const float white = rng.nextBipolar();
        b0 = 0.99765f * b0 + white * 0.0990460f;
        b1 = 0.96300f * b1 + white * 0.2965164f;
        b2 = 0.57000f * b2 + white * 1.0526913f;
        // The Kellett filter has a lot of gain at the bottom; 0.2 brings the
        // peak back to roughly the requested amplitude.
        out[i] = amplitude * (b0 + b1 + b2 + white * 0.1848f) * 0.2f;
    }
    return out;
}

/** Logarithmic sweep, useful for eyeballing the MODEL profiles in a spectrogram. */
inline std::vector<float> logSweep (size_t n, float startHz, float endHz, double sampleRate,
                                    float amplitude = 0.5f)
{
    std::vector<float> out (n);
    const double duration = static_cast<double> (n) / sampleRate;
    const double k = std::log (static_cast<double> (endHz) / static_cast<double> (startHz));

    for (size_t i = 0; i < n; ++i)
    {
        const double t = static_cast<double> (i) / sampleRate;
        const double phase = 2.0 * kPiD * static_cast<double> (startHz) * duration / k
                             * (std::exp (k * t / duration) - 1.0);
        out[i] = amplitude * static_cast<float> (std::sin (phase));
    }
    return out;
}

/** A repeating percussive hit -- the signal that makes dropouts and snags most
    obvious when listening to the dumped WAVs. */
inline std::vector<float> percussive (size_t n, double sampleRate, float bpm = 100.0f)
{
    std::vector<float> out (n, 0.0f);
    const size_t period = static_cast<size_t> (sampleRate * 60.0 / static_cast<double> (bpm));
    TestRng rng (99u);

    for (size_t i = 0; i < n; ++i)
    {
        const size_t inBeat = (period > 0) ? (i % period) : i;
        const double t = static_cast<double> (inBeat) / sampleRate;
        const float env = static_cast<float> (std::exp (-t * 14.0));
        const float tone = static_cast<float> (std::sin (2.0 * kPiD * 180.0 * t))
                           + 0.4f * rng.nextBipolar() * static_cast<float> (std::exp (-t * 60.0));
        out[i] = 0.6f * env * tone;
    }
    return out;
}

} // namespace bdvhs::test
