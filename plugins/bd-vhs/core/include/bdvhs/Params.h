#pragma once

#include <cstdint>

namespace bdvhs
{

/** Performance effect fired by the AUX footswitch. */
enum class AuxMode : int
{
    Stop = 0,   ///< Tape stop: pitch and level wind down, then wind back up.
    Filter,     ///< Momentarily bypass the MODEL bandwidth restriction.
    Fail        ///< Force FAILURE to maximum while held.
};

/** How much unprocessed signal is blended back in. */
enum class DryLevel : int
{
    None = 0,   ///< Pure machine.
    Small,      ///< A touch of clean; the machine still dominates.
    Unity       ///< Clean matches input level. Chorusing territory.
};

enum class NoiseLevel : int   { Off = 0, Low, High };

/** How the hiss reacts to program material. */
enum class NoiseResponse : int
{
    Static = 0, ///< Constant.
    Gated,      ///< Hiss appears only when something is playing.
    Ducked      ///< Hiss is loudest in the gaps.
};

/** Whether the dry blend is untouched, or shares everything except wow/flutter. */
enum class DryType : int { Clean = 0, Processed };

enum class RampMode : int { Off = 0, Ramp, Bounce };

/**
    Plain-old-data parameter block handed to Core::setParams() once per audio
    block. Deliberately free of any framework type so the DSP core can be built
    and tested without JUCE.

    Percentages are 0..100 rather than 0..1 to match the plugin parameters
    one-for-one; the core normalises internally.
*/
struct Params
{
    float wow          = 25.0f;   ///< 0..100 %
    float flutter      = 20.0f;   ///< 0..100 %
    float model        = 30.0f;   ///< 0..100 %, morphs the machine profile table
    float saturate     = 30.0f;   ///< 0..100 %
    float failure      = 15.0f;   ///< 0..100 %
    float volumeDb     =  0.0f;   ///< -60..+6 dB
    float mix          = 100.0f;  ///< 0..100 % wet
    float inputGainDb  =  0.0f;   ///< -12..+12 dB
    float stopTimeSec  =  0.6f;   ///< 0.05..4.0 s, AUX = Stop wind-down time
    float rampTimeSec  =  2.0f;   ///< 0.05..20.0 s

    AuxMode       auxMode       = AuxMode::Stop;
    bool          auxHeld       = false;   ///< momentary: true while the footswitch is down
    DryLevel      dryLevel      = DryLevel::Small;
    NoiseLevel    noise         = NoiseLevel::Low;
    NoiseResponse noiseResponse = NoiseResponse::Gated;
    DryType       dryType       = DryType::Clean;
    RampMode      rampMode      = RampMode::Off;

    bool spread    = false;   ///< FAILURE also destabilises the stereo image
    bool modelSnap = false;   ///< step between machines instead of morphing
    bool bypass    = false;
};

} // namespace bdvhs
