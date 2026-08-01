#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace bdvhs::ids
{

// Parameter IDs are permanent: hosts store them in saved sessions, so renaming
// one silently drops that parameter's automation on every existing project.
inline constexpr const char* wow           = "wow";
inline constexpr const char* flutter       = "flutter";
inline constexpr const char* model         = "model";
inline constexpr const char* saturate      = "saturate";
inline constexpr const char* failure       = "failure";
inline constexpr const char* volume        = "volume";
inline constexpr const char* mix           = "mix";
inline constexpr const char* inputGain     = "input_gain";

inline constexpr const char* auxMode       = "aux_mode";
inline constexpr const char* aux           = "aux";
inline constexpr const char* dryLevel      = "dry_level";
inline constexpr const char* noise         = "noise";

inline constexpr const char* stopTime      = "stop_time";
inline constexpr const char* rampTime      = "ramp_time";
inline constexpr const char* bypass        = "bypass";

// The hidden dip switches. Non-automatable: they configure the machine rather
// than perform with it, and they would only clutter automation lanes.
inline constexpr const char* noiseResponse = "noise_resp";
inline constexpr const char* dryType       = "dry_type";
inline constexpr const char* spread        = "spread";
inline constexpr const char* modelSnap     = "model_snap";
inline constexpr const char* rampMode      = "ramp_mode";

} // namespace bdvhs::ids

namespace bdvhs
{

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace bdvhs
