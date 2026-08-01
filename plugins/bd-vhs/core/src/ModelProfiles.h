#pragma once

#include "dsp/Common.h"

namespace bdvhs
{

/**
    The frequency-domain fingerprint of one class of recording machine.

    These are design parameters, not filter coefficients, and that distinction
    is load-bearing: MODEL morphs continuously between adjacent profiles, and
    interpolating design parameters keeps every intermediate point a valid
    filter design. Interpolating biquad coefficients directly can land a pole
    pair outside the stability triangle.

    Provenance: these are designed by ear from the characteristic response of
    each machine class, not measured from any specific product. tools/
    fit_profile.py documents how to regenerate a row from a real swept-sine
    measurement if you have the hardware.
*/
struct MachineProfile
{
    const char* name;

    float hpHz,     hpQ;              ///< bass loss
    float lfBumpHz, lfBumpDb, lfBumpQ; ///< head bump
    float tiltDb;                     ///< broadband tilt about 900 Hz
    float presHz,   presDb,   presQ;   ///< presence honk (+) or dip (-)
    float lpHz,     lpQ;              ///< bandwidth ceiling; Q > 0.707 gives a resonant horn
    float satBias;                    ///< profile-dependent extra saturation drive
    float noiseTiltDb;                ///< hiss colouration
    float outGainDb;                  ///< loudness match across the morph
};

inline constexpr int kNumProfiles = 8;

inline const MachineProfile kProfiles[kNumProfiles] = {
    // name              hpHz  hpQ   bumpHz bumpDb bumpQ  tilt  presHz presDb presQ  lpHz    lpQ   satBias noiseTilt outDb
    { "Studio",           25.f, 0.70f,  75.f, +2.0f, 0.9f, +0.5f, 3000.f, -1.0f, 0.8f, 17000.f, 0.60f, 0.85f,  0.0f,  0.0f },
    { "Cassette",         45.f, 0.70f,  90.f, +1.5f, 1.0f, -1.0f, 4000.f, -2.0f, 1.0f, 13000.f, 0.70f, 0.95f, -1.0f, +0.5f },
    { "VHS SP",           60.f, 0.70f, 110.f, +1.0f, 1.0f, -1.5f, 3500.f, -3.0f, 1.1f, 11000.f, 0.80f, 1.00f, -2.0f, +1.0f },
    { "VHS EP",          110.f, 0.80f, 140.f, +1.5f, 1.1f, -3.5f, 2600.f, -4.5f, 1.2f,  6500.f, 0.95f, 1.10f, -3.5f, +2.5f },
    { "Camcorder",       180.f, 0.90f, 200.f, +0.5f, 1.0f, -4.5f, 2200.f, +2.5f, 1.4f,  5200.f, 1.10f, 1.20f, -5.0f, +3.5f },
    { "Answer Machine",  300.f, 1.00f, 330.f, -1.0f, 1.0f, -5.5f, 1600.f, +4.0f, 1.6f,  3400.f, 1.25f, 1.30f, -6.5f, +5.0f },
    { "Dictaphone",      420.f, 1.10f, 450.f, -2.0f, 1.0f, -7.0f, 1400.f, +5.0f, 1.8f,  2600.f, 1.40f, 1.40f, -8.0f, +6.5f },
    { "Toy",             700.f, 1.20f, 750.f, -3.0f, 1.0f, -8.0f, 1150.f, +6.0f, 2.2f,  1800.f, 1.60f, 1.50f, -9.5f, +8.0f },
};

/**
    Produces the profile for a MODEL position in 0..1.

    Frequencies and Qs interpolate geometrically, decibel quantities linearly --
    the domains in which each is perceptually uniform, and in which every
    intermediate value remains a legal filter design.
*/
inline MachineProfile interpolateProfile (float model01, bool snap) noexcept
{
    const float p = clampf (model01, 0.0f, 1.0f) * static_cast<float> (kNumProfiles - 1);

    int   i = static_cast<int> (p);
    float t = p - static_cast<float> (i);

    if (i >= kNumProfiles - 1)
    {
        i = kNumProfiles - 2;
        t = 1.0f;
    }

    if (snap)
        t = (t < 0.5f) ? 0.0f : 1.0f;

    const MachineProfile& a = kProfiles[i];
    const MachineProfile& b = kProfiles[i + 1];

    MachineProfile out {};
    out.name        = (t < 0.5f) ? a.name : b.name;

    out.hpHz        = lerpLog (a.hpHz,     b.hpHz,     t);
    out.hpQ         = lerpLog (a.hpQ,      b.hpQ,      t);
    out.lfBumpHz    = lerpLog (a.lfBumpHz, b.lfBumpHz, t);
    out.lfBumpDb    = lerp    (a.lfBumpDb, b.lfBumpDb, t);
    out.lfBumpQ     = lerpLog (a.lfBumpQ,  b.lfBumpQ,  t);
    out.tiltDb      = lerp    (a.tiltDb,   b.tiltDb,   t);
    out.presHz      = lerpLog (a.presHz,   b.presHz,   t);
    out.presDb      = lerp    (a.presDb,   b.presDb,   t);
    out.presQ       = lerpLog (a.presQ,    b.presQ,    t);
    out.lpHz        = lerpLog (a.lpHz,     b.lpHz,     t);
    out.lpQ         = lerpLog (a.lpQ,      b.lpQ,      t);
    out.satBias     = lerp    (a.satBias,  b.satBias,  t);
    out.noiseTiltDb = lerp    (a.noiseTiltDb, b.noiseTiltDb, t);
    out.outGainDb   = lerp    (a.outGainDb,   b.outGainDb,   t);

    return out;
}

} // namespace bdvhs
