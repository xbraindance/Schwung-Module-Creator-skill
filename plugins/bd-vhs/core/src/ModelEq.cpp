#include "ModelEq.h"

namespace bdvhs
{

namespace
{
    /** The broadband tilt pivots here: low shelf down and high shelf up by half
        the tilt each, so the response rotates about roughly this frequency. */
    constexpr float kTiltPivotHz = 900.0f;
}

void ModelEq::prepare (double sampleRate)
{
    sr = sampleRate;
    reset();
    setProfile (kProfiles[0]);
}

void ModelEq::reset()
{
    highpass.reset();
    lfBump.reset();
    tiltLow.reset();
    tiltHigh.reset();
    presence.reset();
    lowpass.reset();
}

void ModelEq::setProfile (const MachineProfile& p) noexcept
{
    const float srf = static_cast<float> (sr);
    const float nyquistLimit = srf * 0.48f;

    highpass.setHighpass (clampf (p.hpHz, 10.0f, nyquistLimit), p.hpQ, srf);

    lfBump.setBell (clampf (p.lfBumpHz, 20.0f, nyquistLimit),
                    p.lfBumpQ, p.lfBumpDb, srf);

    tiltLow.setLowShelf   (kTiltPivotHz, 0.707f, -p.tiltDb * 0.5f, srf);
    tiltHigh.setHighShelf (kTiltPivotHz, 0.707f, +p.tiltDb * 0.5f, srf);

    presence.setBell (clampf (p.presHz, 100.0f, nyquistLimit),
                      p.presQ, p.presDb, srf);

    lowpass.setLowpass (clampf (p.lpHz, 200.0f, nyquistLimit), p.lpQ, srf);

    outGain = dbToGain (p.outGainDb);
}

float ModelEq::process (float x) noexcept
{
    float y = highpass.process (x);
    y = lfBump.process (y);
    y = tiltLow.process (y);
    y = tiltHigh.process (y);
    y = presence.process (y);
    y = lowpass.process (y);
    return y * outGain;
}

} // namespace bdvhs
