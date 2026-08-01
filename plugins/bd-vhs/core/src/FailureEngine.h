#pragma once

#include "dsp/Random.h"

namespace bdvhs
{

/** Everything one channel's malfunctions do to the signal in a given sample. */
struct FailureOutput
{
    float gain                = 1.0f;      ///< multiplicative dropout / wrinkle AM
    float delayOffsetSamples  = 0.0f;      ///< pitch snag, added to the tape delay
    float lpCutoffHz          = 20000.0f;  ///< wrinkle sweep
    float crackle             = 0.0f;      ///< additive tick
    bool  snagActive          = false;     ///< relaxes the delay-time slew clamp
};

/**
    The little things that go wrong: dropouts where the tape loses the head,
    snags where it slips, wrinkles that smear the top end, and the occasional
    crackle of physical damage.

    Events arrive as a Poisson process whose rate rises with the FAILURE knob.
    Everything is drawn from a seeded xoshiro stream, so a given seed always
    produces exactly the same sequence -- which is what makes the statistical
    tests meaningful and a rendered bounce reproducible.
*/
class FailureEngine
{
public:
    void prepare (double sampleRate, uint32_t seed, int channelIndex);
    void reset();

    /** Control-rate. `failure01` is the knob normalised to 0..1. */
    void setAmount (float failure01) noexcept;

    /** Advances one sample. */
    void process (FailureOutput& out) noexcept;

    /** Number of events started since the last reset. Used by the statistical
        test; not needed by the audio path. */
    int eventCount() const noexcept { return eventsStarted; }

private:
    enum class EventType { Dropout, Snag, Wrinkle, Crackle };

    struct Event
    {
        bool  active = false;
        EventType type = EventType::Dropout;

        float phase = 0.0f;        // 0..1 across the whole event
        float phaseInc = 0.0f;

        float attackFrac = 0.0f;   // dropout envelope segmentation
        float holdFrac = 0.0f;

        float depthGain = 1.0f;    // dropout floor
        float rate = 1.0f;         // snag playback rate
        float crackleFreq = 2000.0f;
        float crackleDecay = 0.0f;
        int   cracklesLeft = 0;
        float crackleTimer = 0.0f;
        float cracklePhase = 0.0f;
        float crackleEnv = 0.0f;
    };

    void startEvent (Event& e) noexcept;
    void scheduleNext() noexcept;

    static constexpr int kMaxEvents = 3;

    double sr = 44100.0;
    Xoshiro128 rng;

    Event events[kMaxEvents];
    float samplesUntilNext = 0.0f;
    float amount = 0.0f;
    float rateHz = 0.02f;

    float snagOffset = 0.0f;
    float snagDecay = 0.0f;

    QuadOsc wrinkleAm;

    int eventsStarted = 0;
};

} // namespace bdvhs
