#pragma once

#include <bdvhs/Params.h>

#include "ModelProfiles.h"
#include "dsp/Random.h"
#include "dsp/Smoothing.h"
#include "dsp/Svf.h"

namespace bdvhs
{

/**
    Tape hiss and machine hum, for one channel.

    The hiss is band-limited by the *same* profile that shapes the signal, which
    is the detail that makes it convincing: a dictaphone's noise floor must not
    carry 15 kHz content, or the ear immediately hears two unrelated things
    stacked rather than one machine.
*/
class NoiseEngine
{
public:
    void prepare (double sampleRate, uint32_t seed, int channelIndex);
    void reset();

    /** Control-rate. */
    void setProfile (const MachineProfile& profile, NoiseLevel level) noexcept;

    /** Advances one sample.
        @param responseGain  0..1 from the noise-response follower, computed
                             once for the whole instance by Core.
    */
    float process (float responseGain) noexcept;

private:
    double sr = 44100.0;

    Xoshiro128 rng;
    PinkNoise pink;

    Svf highpass;
    Svf lowpass;

    QuadOsc hum50;
    QuadOsc hum100;

    Smoother level;
    float humGain = 0.0f;
};

} // namespace bdvhs
