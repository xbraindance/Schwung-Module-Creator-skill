#include "AuxPerformance.h"

namespace bdvhs
{

void AuxPerformance::prepare (double sampleRate)
{
    sr = sampleRate;

    filterFade.setTimeMs (8.0f, sr);
    failFade.setTimeMs (30.0f, sr);
    filterFade.reset (0.0f);
    failFade.reset (0.0f);

    reset();
}

void AuxPerformance::reset()
{
    stopPhase = 0.0f;
    rampPhase = 0.0f;
    bounceRising = true;
    filterFade.reset (0.0f);
    failFade.reset (0.0f);
}

void AuxPerformance::update (const Params& p) noexcept
{
    mode = p.auxMode;
    rampMode = p.rampMode;
    held = p.auxHeld;

    const float stopSamples = std::fmax (1.0f, p.stopTimeSec * static_cast<float> (sr));
    stopInc = 1.0f / stopSamples;
    restartInc = 2.0f / stopSamples;   // winds back up twice as fast as it wound down

    const float rampSamples = std::fmax (1.0f, p.rampTimeSec * static_cast<float> (sr));
    rampInc = 1.0f / rampSamples;

    const bool rampActive = (rampMode != RampMode::Off);

    filterFade.setTarget ((! rampActive && held && mode == AuxMode::Filter) ? 1.0f : 0.0f);
    failFade.setTarget   ((! rampActive && held && mode == AuxMode::Fail)   ? 1.0f : 0.0f);
}

void AuxPerformance::process (AuxState& out) noexcept
{
    const bool rampActive = (rampMode != RampMode::Off);

    // ---- tape stop -------------------------------------------------------
    if (! rampActive && mode == AuxMode::Stop && held)
        stopPhase = std::fmin (1.0f, stopPhase + stopInc);
    else
        stopPhase = std::fmax (0.0f, stopPhase - restartInc);

    if (stopPhase > 0.0f)
    {
        // Ease the wind-down so the pitch drop accelerates the way a transport
        // losing torque actually does.
        const float eased = stopPhase * stopPhase;
        out.rateDeviation = eased;                       // playback rate 1 -> 0
        out.levelGain = std::sqrt (std::fmax (0.0f, 1.0f - eased));
    }
    else
    {
        out.rateDeviation = 0.0f;
        out.levelGain = 1.0f;
    }

    out.filterBypass = filterFade.next();
    out.failureBoost = failFade.next();

    // ---- ramp / bounce ---------------------------------------------------
    if (rampActive && held)
    {
        if (rampMode == RampMode::Ramp)
        {
            rampPhase = std::fmin (1.0f, rampPhase + rampInc);
        }
        else
        {
            // Bounce: a triangle that turns around at each end and keeps going
            // for as long as the switch is held.
            rampPhase += bounceRising ? rampInc : -rampInc;
            if (rampPhase >= 1.0f) { rampPhase = 1.0f; bounceRising = false; }
            if (rampPhase <= 0.0f) { rampPhase = 0.0f; bounceRising = true;  }
        }
    }
    else if (rampPhase > 0.0f)
    {
        rampPhase = std::fmax (0.0f, rampPhase - rampInc);
        bounceRising = true;
    }

    out.ramp = rampPhase;
}

} // namespace bdvhs
