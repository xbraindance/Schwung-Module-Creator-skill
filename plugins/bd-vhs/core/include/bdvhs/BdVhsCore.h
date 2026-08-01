#pragma once

#include <bdvhs/Params.h>

#include <cstdint>
#include <memory>

namespace bdvhs
{

/**
    BD-VHS tape / video-cassette degradation.

    The whole effect lives behind this one class, with no dependency on JUCE or
    any other framework, so it can be built and exercised on a machine that has
    no GUI or audio development headers installed.

    Real-time contract:
      - prepare() allocates. Everything else is allocation-free and lock-free.
      - process() works in place and must be called from the audio thread only.
      - setParams() may be called once per block, immediately before process().

    Determinism: given the same seed and the same sequence of setParams()/
    process() calls, output is bit-identical. All randomness comes from a
    seeded xoshiro128+ stream, never from rand() or std::random_device.
*/
class Core
{
public:
    Core();
    ~Core();

    Core (const Core&)            = delete;
    Core& operator= (const Core&) = delete;

    /** Allocates all internal storage. Must be called before process().
        @param sampleRate       host sample rate in Hz
        @param maxBlockSamples  largest block process() will ever be given
        @param numChannels      1 or 2
    */
    void prepare (double sampleRate, int maxBlockSamples, int numChannels);

    /** Clears all state without deallocating. Safe on the audio thread. */
    void reset();

    /** Reseeds every random stream. Intended for tests and for making a
        rendered bounce reproducible. */
    void setSeed (uint32_t seed);

    /** Copies the parameter block. Cheap; no allocation, no locking. */
    void setParams (const Params& p);

    /** Processes in place.
        @param io           array of numChannels pointers to numSamples floats
        @param numChannels  must not exceed the value passed to prepare()
        @param numSamples   must not exceed maxBlockSamples
    */
    void process (float* const* io, int numChannels, int numSamples);

    /** Latency introduced by the saturation stage's 2x oversampler, in samples
        at the host rate. Constant at every sample rate by construction, so the
        value reported to the host never changes at runtime.

        Note this does NOT include the tape path's ~16 ms record-to-playback
        gap. That gap only applies to the wet signal -- the dry blend is aligned
        to the input -- and reporting it would make the plugin's dry path lead
        the beat by 16 ms in every host. See README.
    */
    static constexpr int latencySamples() { return 15; }

    /** Length of the internal control-rate grid, in samples. Coefficients and
        smoothers update on this fixed grid measured from the start of the
        stream, not from the start of each host block, which is what makes
        output independent of the host's buffer size. */
    static constexpr int controlBlockSamples() { return 32; }

    /** Diagnostic only: how many failure events have started since the last
        reset(), summed across channels. The audio path does not use this; the
        test suite checks it against the expected Poisson statistics. */
    int failureEventCount() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace bdvhs
