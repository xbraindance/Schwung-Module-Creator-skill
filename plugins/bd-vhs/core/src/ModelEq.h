#pragma once

#include "ModelProfiles.h"
#include "dsp/Svf.h"

namespace bdvhs
{

/**
    The bandwidth and colouration of one machine, for one lane.

    Six trapezoidal state-variable sections in series. TPT rather than a direct
    form specifically because MODEL is a continuously swept control: TPT state
    is a pair of integrator values with physical meaning, so recomputing
    coefficients underneath a running filter does not produce the state-mismatch
    artefacts a direct-form biquad would.
*/
class ModelEq
{
public:
    void prepare (double sampleRate);
    void reset();

    /** Control-rate. Takes an already-interpolated profile so the whole core
        shares a single interpolation per block. */
    void setProfile (const MachineProfile& profile) noexcept;

    float process (float x) noexcept;

private:
    double sr = 44100.0;

    Svf highpass;
    Svf lfBump;
    Svf tiltLow;
    Svf tiltHigh;
    Svf presence;
    Svf lowpass;

    float outGain = 1.0f;
};

} // namespace bdvhs
