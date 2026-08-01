#include "NoiseEngine.h"

namespace bdvhs
{

namespace
{
    constexpr float kHissHpHz = 200.0f;

    /** Output hiss level for each toggle position, in dBFS. */
    constexpr float kLevelLowDb  = -72.0f;
    constexpr float kLevelHighDb = -54.0f;

    constexpr float kHumHz = 60.0f;
    constexpr float kHumRelativeDb = -40.0f;

    /** Brings the pink generator to roughly unity RMS so that the dBFS figures
        above mean approximately what they say once band-limiting is applied. */
    constexpr float kPinkMakeup = 3.0f;
}

void NoiseEngine::prepare (double sampleRate, uint32_t seed, int channelIndex)
{
    sr = sampleRate;
    rng.setSeed (seed ^ (0xC2B2AE35u * static_cast<uint32_t> (channelIndex + 1)));

    hum50.prepare (sr, kHumHz, 0.11f * static_cast<float> (channelIndex));
    hum100.prepare (sr, kHumHz * 2.0f, 0.23f * static_cast<float> (channelIndex));

    level.setTimeMs (50.0f, sr);
    level.reset (0.0f);

    highpass.setHighpass (kHissHpHz, 0.707f, static_cast<float> (sr));
    setProfile (kProfiles[0], NoiseLevel::Off);

    reset();
}

void NoiseEngine::reset()
{
    pink.reset();
    highpass.reset();
    lowpass.reset();
    level.reset (level.getTarget());
}

void NoiseEngine::setProfile (const MachineProfile& profile, NoiseLevel noiseLevel) noexcept
{
    const float srf = static_cast<float> (sr);

    // Band-limited by the machine it belongs to.
    lowpass.setLowpass (clampf (profile.lpHz * 0.9f, 200.0f, srf * 0.45f), 0.707f, srf);

    float db;
    switch (noiseLevel)
    {
        case NoiseLevel::Low:  db = kLevelLowDb;  break;
        case NoiseLevel::High: db = kLevelHighDb; break;
        case NoiseLevel::Off:
        default:               db = -200.0f;     break;
    }

    const float gain = (noiseLevel == NoiseLevel::Off)
                         ? 0.0f
                         : dbToGain (db + profile.noiseTiltDb * 0.5f);

    level.setTarget (gain);
    humGain = dbToGain (kHumRelativeDb);
}

float NoiseEngine::process (float responseGain) noexcept
{
    const float g = level.next();

    // Keep the generators running even when muted so that toggling NOISE does
    // not restart a recognisable noise pattern.
    float hiss = lowpass.process (highpass.process (pink.next (rng) * kPinkMakeup));

    const float hum = humGain * (hum50.sine() + 0.5f * hum100.sine());
    hum50.tick();
    hum100.tick();

    if (g <= 0.0f)
        return 0.0f;

    return (hiss + hum) * g * responseGain;
}

} // namespace bdvhs
