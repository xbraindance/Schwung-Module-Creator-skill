#pragma once

#include "ModelProfiles.h"
#include "dsp/HalfBand.h"
#include "dsp/Svf.h"
#include "dsp/Smoothing.h"

namespace bdvhs
{

/**
    Magnetic saturation, for one lane.

    The pre/de-emphasis pair around the nonlinearity is what makes this read as
    tape rather than as a transistor: high frequencies are lifted into the
    nonlinearity, compressed harder than the rest of the spectrum, then
    restored. That HF squash is the audible signature. The small squared term
    supplies tape's characteristic second harmonic, and the DC blocker cleans up
    the offset that asymmetry introduces.

    Runs 2x oversampled at all times, so reported latency is a constant and
    never changes at runtime.
*/
class Saturator
{
public:
    static constexpr int latencySamples() { return HalfBand2x::latencySamples(); }

    void prepare (double sampleRate);
    void reset();

    /** Control-rate. `saturate01` is the knob normalised to 0..1. */
    void setDrive (float saturate01, float profileBias) noexcept;

    float process (float x) noexcept;

private:
    float shape (float x) const noexcept;

    double sr = 44100.0;

    HalfBand2x halfBand;
    Svf preEmphasis, deEmphasis;   // run at the oversampled rate
    DcBlocker dcBlock;

    Smoother preGain, postGain;
};

} // namespace bdvhs
