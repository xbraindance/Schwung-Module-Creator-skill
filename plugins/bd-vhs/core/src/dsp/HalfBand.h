#pragma once

#include "Common.h"

namespace bdvhs
{

/**
    2x oversampler built from a 31-tap Kaiser-windowed half-band FIR, in
    polyphase form.

    Owning this rather than using a framework oversampler buys two things the
    core cares about: it allocates nothing, and its latency is a compile-time
    constant of exactly 15 samples at the host rate for every sample rate. A
    latency figure that changes at runtime is a well-known way to break hosts.

    Half-band structure: with 31 taps centred at 15, every tap an even distance
    from the centre is zero except the centre itself (0.5). So the odd polyphase
    branch degenerates to a pure 7-sample delay and only the 16 even taps cost
    any multiplies.
*/
class HalfBand2x
{
public:
    static constexpr int kNumTaps    = 31;
    static constexpr int kCentre     = 15;
    static constexpr int kEvenTaps   = 16;   // h[0], h[2], ... h[30]
    static constexpr int kHistory    = 32;   // power of two, comfortably > kEvenTaps
    static constexpr int kHistMask   = kHistory - 1;

    /** Group delay of an up/downsample round trip, in samples at the base rate. */
    static constexpr int latencySamples() { return 15; }

    HalfBand2x() { designTaps(); }

    void reset() noexcept
    {
        for (int i = 0; i < kHistory; ++i)
            upHist[i] = downEven[i] = downOdd[i] = 0.0f;
        upIndex = downIndex = 0;
    }

    /** Consumes one base-rate sample, produces two at 2x. */
    void upsample (float x, float& out0, float& out1) noexcept
    {
        upIndex = (upIndex + 1) & kHistMask;
        upHist[upIndex] = x;

        float acc = 0.0f;
        for (int j = 0; j < kEvenTaps; ++j)
            acc += evenTaps[j] * upHist[(upIndex - j) & kHistMask];

        // Even phase carries the filtered branch, odd phase the bare delay.
        out0 = 2.0f * acc;
        out1 = upHist[(upIndex - 7) & kHistMask];
    }

    /** Consumes two 2x samples, produces one at the base rate. */
    float downsample (float in0, float in1) noexcept
    {
        downIndex = (downIndex + 1) & kHistMask;
        downEven[downIndex] = in0;
        downOdd[downIndex]  = in1;

        float acc = 0.0f;
        for (int j = 0; j < kEvenTaps; ++j)
            acc += evenTaps[j] * downEven[(downIndex - j) & kHistMask];

        return acc + 0.5f * downOdd[(downIndex - 8) & kHistMask];
    }

private:
    void designTaps() noexcept
    {
        // Kaiser beta 8.0 gives roughly -80 dB of stopband rejection, which is
        // far below anything the saturator's own harmonics will produce.
        constexpr float beta = 8.0f;
        const float denom = besselI0 (beta);

        float taps[kNumTaps] {};
        for (int n = 0; n < kNumTaps; ++n)
        {
            const float d = static_cast<float> (n - kCentre);
            const float ideal = 0.5f * sincf (0.5f * d);

            const float r = (2.0f * static_cast<float> (n) / static_cast<float> (kNumTaps - 1)) - 1.0f;
            const float w = besselI0 (beta * std::sqrt (std::fmax (0.0f, 1.0f - r * r))) / denom;

            taps[n] = ideal * w;
        }

        float sum = 0.0f;
        for (int j = 0; j < kEvenTaps; ++j)
            sum += taps[2 * j];

        // Normalise the even branch to 0.5 so that both the upsampler
        // (2 * 0.5 = 1) and the downsampler (0.5 + 0.5 = 1) are unity at DC.
        const float scale = 0.5f / sum;
        for (int j = 0; j < kEvenTaps; ++j)
            evenTaps[j] = taps[2 * j] * scale;
    }

    static float sincf (float x) noexcept
    {
        if (std::fabs (x) < 1.0e-7f)
            return 1.0f;
        const float px = kPi * x;
        return std::sin (px) / px;
    }

    static float besselI0 (float x) noexcept
    {
        float sum = 1.0f, term = 1.0f;
        const float halfSq = 0.25f * x * x;
        for (int k = 1; k < 40; ++k)
        {
            term *= halfSq / (static_cast<float> (k) * static_cast<float> (k));
            sum += term;
            if (term < 1.0e-12f * sum)
                break;
        }
        return sum;
    }

    float evenTaps[kEvenTaps] {};
    float upHist[kHistory] {};
    float downEven[kHistory] {};
    float downOdd[kHistory] {};
    int upIndex = 0, downIndex = 0;
};

} // namespace bdvhs
