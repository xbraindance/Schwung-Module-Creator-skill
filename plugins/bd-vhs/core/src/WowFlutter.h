#pragma once

#include "dsp/Random.h"
#include "dsp/Smoothing.h"

namespace bdvhs
{

/**
    Tape transport instability for one channel: the slow smooth drift of wow
    and the fast twitchy wobble of flutter, plus the amplitude component that
    comes with the latter as the tape loses contact with the head.

    Produces a delay-time deviation in samples, to be added to the tape path's
    nominal record-to-playback gap, and a gain multiplier.
*/
class WowFlutter
{
public:
    /** Nominal record-to-playback gap. Long enough that full wow deviation
        never drives the read pointer past the write pointer, short enough that
        blending dry back in reads as chorus rather than slapback. */
    static constexpr float kBaseDelayMs = 16.0f;
    static constexpr float kMaxWowMs    =  7.0f;
    static constexpr float kMaxFlutterMs = 0.35f;

    void prepare (double sampleRate, uint32_t seed, int channelIndex);
    void reset();

    /** Control-rate. Depths are normalised 0..1. */
    void setDepth (float wow01, float flutter01) noexcept;

    /** Advances one sample.
        @param delayDeviationSamples  deviation from the nominal gap, in samples
        @param amplitudeGain          flutter's amplitude component
    */
    void process (float& delayDeviationSamples, float& amplitudeGain) noexcept;

private:
    double sr = 44100.0;

    // Wow: three incommensurate slow sines plus a slow random drift.
    QuadOsc wowOsc[3];
    SmoothRandom wowDrift;

    // Flutter: two fast sines whose ratio is close to root two, so the beat
    // pattern never audibly repeats, plus jitter and a high scrape component.
    QuadOsc flutterOsc[2];
    QuadOsc scrapeOsc;
    SmoothRandom flutterJitter;

    Xoshiro128 rng;

    Smoother wowDepth, flutterDepth;

    float wowScale = 0.0f;       // samples per unit of normalised wow signal
    float flutterScale = 0.0f;
};

} // namespace bdvhs
