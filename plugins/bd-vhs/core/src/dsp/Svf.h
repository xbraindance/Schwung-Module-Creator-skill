#pragma once

#include "Common.h"

namespace bdvhs
{

/**
    Andy Simper / Cytomic topology-preserving state-variable filter.

    Chosen over Direct Form II specifically because MODEL sweeps its cutoffs
    continuously: TPT state is a pair of trapezoidal integrator values with
    physical meaning, so changing the coefficients between samples does not
    produce the state-mismatch artefacts a direct-form biquad would.

    Coefficients are set from filter *design* parameters (frequency, Q, gain),
    which is what makes them safe to interpolate -- see ModelEq.
*/
class Svf
{
public:
    void reset() noexcept { ic1eq = ic2eq = 0.0f; }

    void setLowpass (float freqHz, float q, float sampleRate) noexcept
    {
        setGK (std::tan (kPi * freqHz / sampleRate), 1.0f / q);
        m0 = 0.0f; m1 = 0.0f; m2 = 1.0f;
    }

    void setHighpass (float freqHz, float q, float sampleRate) noexcept
    {
        const float kk = 1.0f / q;
        setGK (std::tan (kPi * freqHz / sampleRate), kk);
        m0 = 1.0f; m1 = -kk; m2 = -1.0f;
    }

    void setBandpass (float freqHz, float q, float sampleRate) noexcept
    {
        setGK (std::tan (kPi * freqHz / sampleRate), 1.0f / q);
        m0 = 0.0f; m1 = 1.0f; m2 = 0.0f;
    }

    /** Peaking / bell. */
    void setBell (float freqHz, float q, float gainDb, float sampleRate) noexcept
    {
        const float a  = std::pow (10.0f, gainDb * 0.025f);   // sqrt of amplitude
        const float kk = 1.0f / (q * a);
        setGK (std::tan (kPi * freqHz / sampleRate), kk);
        m0 = 1.0f; m1 = kk * (a * a - 1.0f); m2 = 0.0f;
    }

    void setLowShelf (float freqHz, float q, float gainDb, float sampleRate) noexcept
    {
        const float a  = std::pow (10.0f, gainDb * 0.025f);
        const float kk = 1.0f / q;
        setGK (std::tan (kPi * freqHz / sampleRate) / std::sqrt (a), kk);
        m0 = 1.0f; m1 = kk * (a - 1.0f); m2 = a * a - 1.0f;
    }

    void setHighShelf (float freqHz, float q, float gainDb, float sampleRate) noexcept
    {
        const float a  = std::pow (10.0f, gainDb * 0.025f);
        const float kk = 1.0f / q;
        setGK (std::tan (kPi * freqHz / sampleRate) * std::sqrt (a), kk);
        m0 = a * a; m1 = kk * (1.0f - a) * a; m2 = 1.0f - a * a;
    }

    float process (float v0) noexcept
    {
        const float v3 = v0 - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = flushDenormal (2.0f * v1 - ic1eq);
        ic2eq = flushDenormal (2.0f * v2 - ic2eq);

        return m0 * v0 + m1 * v1 + m2 * v2;
    }

private:
    void setGK (float g, float kk) noexcept
    {
        a1 = 1.0f / (1.0f + g * (g + kk));
        a2 = g * a1;
        a3 = g * a2;
    }

    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float m0 = 1.0f, m1 = 0.0f, m2 = 0.0f;
    float ic1eq = 0.0f, ic2eq = 0.0f;
};

/** First-order one-pole highpass, used for DC blocking. */
class DcBlocker
{
public:
    void setCutoff (float freqHz, float sampleRate) noexcept
    {
        const float g = std::tan (kPi * freqHz / sampleRate);
        a = g / (1.0f + g);
    }

    void reset() noexcept { z = 0.0f; }

    float process (float x) noexcept
    {
        const float v = a * (x - z);
        const float lp = v + z;
        z = flushDenormal (lp + v);
        return x - lp;
    }

private:
    float a = 0.0f, z = 0.0f;
};

/** First-order one-pole lowpass with a settable cutoff. Used for the wrinkle
    sweep and for smoothing modulation signals. */
class OnePoleLp
{
public:
    void setCutoff (float freqHz, float sampleRate) noexcept
    {
        const float g = std::tan (kPi * clampf (freqHz, 0.01f, sampleRate * 0.49f) / sampleRate);
        a = g / (1.0f + g);
    }

    void reset (float value = 0.0f) noexcept { z = value; }

    float process (float x) noexcept
    {
        const float v = a * (x - z);
        const float lp = v + z;
        z = flushDenormal (lp + v);
        return lp;
    }

    float current() const noexcept { return z; }

private:
    float a = 0.0f, z = 0.0f;
};

} // namespace bdvhs
