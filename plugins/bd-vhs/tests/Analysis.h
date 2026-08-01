#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace bdvhs::test
{

inline float rms (const std::vector<float>& x, size_t from = 0)
{
    if (from >= x.size())
        return 0.0f;

    double acc = 0.0;
    for (size_t i = from; i < x.size(); ++i)
        acc += static_cast<double> (x[i]) * static_cast<double> (x[i]);

    return static_cast<float> (std::sqrt (acc / static_cast<double> (x.size() - from)));
}

inline float peak (const std::vector<float>& x)
{
    float p = 0.0f;
    for (float v : x)
        p = std::fmax (p, std::fabs (v));
    return p;
}

inline size_t peakIndex (const std::vector<float>& x)
{
    size_t best = 0;
    float bestVal = -1.0f;
    for (size_t i = 0; i < x.size(); ++i)
    {
        const float a = std::fabs (x[i]);
        if (a > bestVal)
        {
            bestVal = a;
            best = i;
        }
    }
    return best;
}

inline float maxAbsDifference (const std::vector<float>& a, const std::vector<float>& b)
{
    const size_t n = std::min (a.size(), b.size());
    float worst = 0.0f;
    for (size_t i = 0; i < n; ++i)
        worst = std::fmax (worst, std::fabs (a[i] - b[i]));
    return worst;
}

inline float maxConsecutiveDelta (const std::vector<float>& x)
{
    float worst = 0.0f;
    for (size_t i = 1; i < x.size(); ++i)
        worst = std::fmax (worst, std::fabs (x[i] - x[i - 1]));
    return worst;
}

inline bool allFinite (const std::vector<float>& x)
{
    for (float v : x)
        if (! std::isfinite (v))
            return false;
    return true;
}

/**
    Amplitude of one frequency bin, via Goertzel over a Hann-windowed segment.

    Hann rather than rectangular because the alias-floor test measures a
    component 3 kHz away from a full-scale tone, and rectangular leakage would
    swamp a -55 dB target.
*/
inline float goertzelAmplitude (const std::vector<float>& x, float freqHz, double sampleRate,
                                size_t from = 0)
{
    if (from >= x.size())
        return 0.0f;

    const size_t n = x.size() - from;
    const double w = 2.0 * M_PI * static_cast<double> (freqHz) / sampleRate;
    const double coeff = 2.0 * std::cos (w);

    double s1 = 0.0, s2 = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double window = 0.5 - 0.5 * std::cos (2.0 * M_PI * static_cast<double> (i)
                                                    / static_cast<double> (n - 1));
        const double s = static_cast<double> (x[from + i]) * window + coeff * s1 - s2;
        s2 = s1;
        s1 = s;
    }

    const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
    // 2/N for the two-sided spectrum, /0.5 for the Hann window's coherent gain.
    return static_cast<float> (4.0 * std::sqrt (std::fmax (0.0, power)) / static_cast<double> (n));
}

inline float correlation (const std::vector<float>& a, const std::vector<float>& b)
{
    const size_t n = std::min (a.size(), b.size());
    double sa = 0.0, sb = 0.0, sab = 0.0;

    for (size_t i = 0; i < n; ++i)
    {
        sa  += static_cast<double> (a[i]) * a[i];
        sb  += static_cast<double> (b[i]) * b[i];
        sab += static_cast<double> (a[i]) * b[i];
    }

    const double denom = std::sqrt (sa * sb);
    return (denom < 1.0e-20) ? 0.0f : static_cast<float> (sab / denom);
}

inline float toDb (float gain)
{
    return 20.0f * std::log10 (std::fmax (gain, 1.0e-12f));
}

} // namespace bdvhs::test
