#pragma once

#include <bdvhs/Params.h>

#include "dsp/Common.h"
#include "dsp/Smoothing.h"

namespace bdvhs
{

/** What the AUX footswitch is doing to the machine right now. */
struct AuxState
{
    float rateDeviation = 0.0f;  ///< 1 - playbackRate, integrated by Core into the tape delay
    float levelGain     = 1.0f;  ///< tape stop also winds the level down
    float filterBypass  = 0.0f;  ///< 0 = MODEL in circuit, 1 = out
    float failureBoost  = 0.0f;  ///< 0..1, blended toward maximum FAILURE
    float ramp          = 0.0f;  ///< 0..1 ramp/bounce position
};

/**
    The footswitch performance effects, and the ramp/bounce automation.

    Ramp and bounce take precedence over the AUX effect when enabled: on the
    pedal both are driven by the same switch, and having one footswitch fire two
    unrelated things at once is not useful. This is a documented deviation.

    Ramp destinations are fixed here (wow, flutter and failure sweep toward
    maximum) because choosing them per-parameter needs a UI to assign them.
*/
class AuxPerformance
{
public:
    void prepare (double sampleRate);
    void reset();

    /** Control-rate. */
    void update (const Params& p) noexcept;

    /** Advances one sample. */
    void process (AuxState& out) noexcept;

private:
    double sr = 44100.0;

    AuxMode mode = AuxMode::Stop;
    RampMode rampMode = RampMode::Off;
    bool held = false;

    float stopPhase = 0.0f;       // 0 = running, 1 = fully stopped
    float stopInc = 0.0f;
    float restartInc = 0.0f;

    float rampPhase = 0.0f;
    float rampInc = 0.0f;
    bool  bounceRising = true;

    Smoother filterFade;
    Smoother failFade;
};

} // namespace bdvhs
