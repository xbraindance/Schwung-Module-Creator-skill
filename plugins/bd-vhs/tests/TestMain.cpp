/*
    BD-VHS core test suite.

    Deliberately dependency-free: no Catch2, no FetchContent, nothing to install.
    Every assertion here is numeric -- finiteness, RMS deltas, peak indices,
    correlations -- so a test framework would buy fixtures and matchers we do
    not need at the cost of a network dependency and a much slower build.

    CHECK() rather than assert(): assert compiles out in Release, and Release is
    exactly where these numeric checks need to run.

    Usage:
        bd_vhs_tests            run the suite
        bd_vhs_tests --dump DIR render a grid of WAVs into DIR for listening
*/

#include <bdvhs/BdVhsCore.h>

#include "Analysis.h"
#include "TestSignals.h"
#include "WavWriter.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

// ============================================================================
// Allocation watchdog. Real-time safety is the kind of regression that is
// invisible until it is a dropout in someone's session, so we catch it
// mechanically: arm the flag, run process(), assert nothing allocated.
// ============================================================================

namespace
{
    std::atomic<bool> gWatchingAllocations { false };
    std::atomic<bool> gAllocationDetected  { false };

    inline void noteAllocation() noexcept
    {
        if (gWatchingAllocations.load (std::memory_order_relaxed))
            gAllocationDetected.store (true, std::memory_order_relaxed);
    }
}

void* operator new (std::size_t n)
{
    noteAllocation();
    void* p = std::malloc (n ? n : 1);
    if (p == nullptr)
        throw std::bad_alloc();
    return p;
}

void* operator new[] (std::size_t n)                       { return ::operator new (n); }
void  operator delete (void* p) noexcept                   { std::free (p); }
void  operator delete[] (void* p) noexcept                 { std::free (p); }
void  operator delete (void* p, std::size_t) noexcept      { std::free (p); }
void  operator delete[] (void* p, std::size_t) noexcept    { std::free (p); }

// ============================================================================
// Harness
// ============================================================================

namespace
{
    int gFailures = 0;
    int gChecks = 0;
    std::string gCurrentTest;

    void check (bool condition, const char* expr, const char* message, int line)
    {
        ++gChecks;
        if (! condition)
        {
            ++gFailures;
            std::printf ("  FAIL  [%s:%d]  %s\n        %s\n",
                         gCurrentTest.c_str(), line, message, expr);
        }
    }
}

#define CHECK(cond, msg) check ((cond), #cond, (msg), __LINE__)

// ============================================================================
// Rendering helpers
// ============================================================================

namespace
{
    using namespace bdvhs;
    using namespace bdvhs::test;

    struct Render
    {
        std::vector<float> l, r;
    };

    /** Baseline parameter set: everything that moves is switched off, so tests
        can turn on exactly the one thing they mean to measure. */
    Params quietParams()
    {
        Params p;
        p.wow = 0.0f;
        p.flutter = 0.0f;
        p.model = 0.0f;
        p.saturate = 0.0f;
        p.failure = 0.0f;
        p.volumeDb = 0.0f;
        p.mix = 100.0f;
        p.inputGainDb = 0.0f;
        p.noise = NoiseLevel::Off;
        p.dryLevel = DryLevel::None;
        p.dryType = DryType::Clean;
        p.noiseResponse = NoiseResponse::Static;
        p.rampMode = RampMode::Off;
        p.spread = false;
        p.modelSnap = false;
        p.bypass = false;
        p.auxHeld = false;
        return p;
    }

    Render render (const Params& p,
                   const std::vector<float>& inL,
                   const std::vector<float>& inR,
                   double sampleRate,
                   int blockSize,
                   int channels,
                   uint32_t seed = 0x5EEDBD42u,
                   bool watchAllocations = false)
    {
        Render out;
        out.l = inL;
        out.r = inR.empty() ? inL : inR;

        Core core;
        core.setSeed (seed);
        core.setParams (p);
        core.prepare (sampleRate, blockSize, channels);

        const int n = static_cast<int> (out.l.size());

        if (watchAllocations)
        {
            gAllocationDetected.store (false);
            gWatchingAllocations.store (true);
        }

        for (int pos = 0; pos < n; pos += blockSize)
        {
            const int count = std::min (blockSize, n - pos);
            float* ptrs[2] = { out.l.data() + pos, out.r.data() + pos };
            core.setParams (p);
            core.process (ptrs, channels, count);
        }

        if (watchAllocations)
            gWatchingAllocations.store (false);

        return out;
    }
}

// ============================================================================
// Tests
// ============================================================================

namespace
{

void testPrepareMatrix()
{
    const double rates[]  = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    const int    blocks[] = { 1, 16, 64, 512, 4096 };

    for (double sr : rates)
    {
        for (int block : blocks)
        {
            Params p;                       // defaults: everything doing something
            const size_t n = static_cast<size_t> (sr * 0.05);
            auto in = pinkNoise (n, 3u);

            auto out = render (p, in, in, sr, block, 2);

            CHECK (allFinite (out.l) && allFinite (out.r),
                   "output must be finite at every sample rate and block size");
            CHECK (peak (out.l) < 4.0f, "output must stay bounded");
        }
    }
}

void testNanFuzz()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 0.5);
    auto in = pinkNoise (n, 5u, 0.6f);

    TestRng rng (4242u);
    bool everyRunFinite = true;
    bool everyRunBounded = true;

    for (int trial = 0; trial < 200; ++trial)
    {
        Params p;
        p.wow         = rng.nextRange (0.0f, 100.0f);
        p.flutter     = rng.nextRange (0.0f, 100.0f);
        p.model       = rng.nextRange (0.0f, 100.0f);
        p.saturate    = rng.nextRange (0.0f, 100.0f);
        p.failure     = rng.nextRange (0.0f, 100.0f);
        p.volumeDb    = rng.nextRange (-60.0f, 6.0f);
        p.mix         = rng.nextRange (0.0f, 100.0f);
        p.inputGainDb = rng.nextRange (-12.0f, 12.0f);
        p.auxMode     = static_cast<AuxMode> (static_cast<int> (rng.nextRange (0.0f, 2.99f)));
        p.auxHeld     = rng.nextRange (0.0f, 1.0f) > 0.5f;
        p.dryLevel    = static_cast<DryLevel> (static_cast<int> (rng.nextRange (0.0f, 2.99f)));
        p.noise       = static_cast<NoiseLevel> (static_cast<int> (rng.nextRange (0.0f, 2.99f)));
        p.noiseResponse = static_cast<NoiseResponse> (static_cast<int> (rng.nextRange (0.0f, 2.99f)));
        p.dryType     = static_cast<DryType> (static_cast<int> (rng.nextRange (0.0f, 1.99f)));
        p.rampMode    = static_cast<RampMode> (static_cast<int> (rng.nextRange (0.0f, 2.99f)));
        p.spread      = rng.nextRange (0.0f, 1.0f) > 0.5f;
        p.modelSnap   = rng.nextRange (0.0f, 1.0f) > 0.5f;

        auto out = render (p, in, in, sr, 128, 2, 0x5EEDBD42u + static_cast<uint32_t> (trial));

        if (! (allFinite (out.l) && allFinite (out.r)))
            everyRunFinite = false;
        if (peak (out.l) >= 4.0f || peak (out.r) >= 4.0f)
            everyRunBounded = false;
    }

    CHECK (everyRunFinite, "no parameter combination may produce NaN or Inf");
    CHECK (everyRunBounded, "no parameter combination may produce unbounded output");
}

void testDenormalGuard()
{
    constexpr double sr = 48000.0;
    const size_t burst = static_cast<size_t> (sr * 0.1);
    const size_t tail  = static_cast<size_t> (sr * 5.0);

    Params p = quietParams();
    p.model = 50.0f;
    p.saturate = 40.0f;

    std::vector<float> in (burst + tail, 0.0f);
    auto hit = sine (burst, 220.0f, sr, 1.0f);
    std::copy (hit.begin(), hit.end(), in.begin());

    const auto start = std::chrono::steady_clock::now();
    auto out = render (p, in, in, sr, 256, 2);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    bool clean = true;
    for (size_t i = burst; i < out.l.size(); ++i)
    {
        const float a = std::fabs (out.l[i]);
        if (a != 0.0f && a < 1.0e-25f)
        {
            clean = false;
            break;
        }
    }
    CHECK (clean, "every sample must be exactly zero or above the denormal floor");

    // A denormal storm shows up as a large, otherwise inexplicable slowdown.
    auto noise = pinkNoise (burst + tail, 9u);
    const auto start2 = std::chrono::steady_clock::now();
    auto out2 = render (p, noise, noise, sr, 256, 2);
    const auto elapsed2 = std::chrono::steady_clock::now() - start2;
    (void) out2;

    const double silentMs = std::chrono::duration<double, std::milli> (elapsed).count();
    const double noisyMs  = std::chrono::duration<double, std::milli> (elapsed2).count();
    CHECK (silentMs < noisyMs * 3.0 + 50.0,
           "processing silence must not be dramatically slower than processing noise");
}

void testNullTransparency()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 0.5);
    auto in = pinkNoise (n, 13u, 0.5f);

    Params p;                       // full defaults, but no wet signal at all
    p.mix = 0.0f;
    p.volumeDb = 0.0f;
    p.bypass = false;

    auto out = render (p, in, in, sr, 64, 2);

    const int latency = Core::latencySamples();
    float worst = 0.0f;
    for (size_t i = static_cast<size_t> (latency); i < n; ++i)
        worst = std::fmax (worst, std::fabs (out.l[i] - in[i - static_cast<size_t> (latency)]));

    std::printf ("        (null residual %.1f dBFS)\n", static_cast<double> (toDb (worst)));
    CHECK (toDb (worst) < -90.0f,
           "at mix = 0 the output must be the input delayed by the reported latency");
}

void testWetUnityGain()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 0.5);
    auto in = sine (n, 1000.0f, sr, 0.25f);

    Params p = quietParams();       // no modulation, profile 0, no saturation

    auto out = render (p, in, in, sr, 128, 2);

    const size_t skip = static_cast<size_t> (sr * 0.1);
    const float inAmp  = goertzelAmplitude (in, 1000.0f, sr, skip);
    const float outAmp = goertzelAmplitude (out.l, 1000.0f, sr, skip);

    const float deltaDb = toDb (outAmp) - toDb (inAmp);
    CHECK (std::fabs (deltaDb) < 1.0f,
           "the wet path must be within 1 dB of unity at 1 kHz on the cleanest profile");
}

void testLatency()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 0.2);
    auto in = impulse (n, 0, 1.0f);

    // Dry path: delayed only by the oversampler's latency.
    {
        Params p = quietParams();
        p.mix = 0.0f;
        auto out = render (p, in, in, sr, 64, 2);
        CHECK (peakIndex (out.l) == static_cast<size_t> (Core::latencySamples()),
               "the dry path must be delayed by exactly the reported latency");
    }

    // Wet path: reported latency plus the tape transport's record/playback gap.
    {
        Params p = quietParams();
        p.mix = 100.0f;
        p.dryLevel = DryLevel::None;
        auto out = render (p, in, in, sr, 64, 2);

        const size_t expected = static_cast<size_t> (Core::latencySamples())
                                + static_cast<size_t> (sr * 0.016);
        const size_t actual = peakIndex (out.l);
        const long diff = static_cast<long> (actual) - static_cast<long> (expected);

        CHECK (std::labs (diff) <= 8,
               "the wet path must land at the reported latency plus the 16 ms tape gap");
    }
}

void testBlockSizeInvariance()
{
    constexpr double sr = 44100.0;
    const size_t n = static_cast<size_t> (sr * 5.0);
    auto in = pinkNoise (n, 17u, 0.4f);

    Params p;                       // defaults: wow, flutter, failure, noise all live

    auto small = render (p, in, in, sr, 64, 2);
    auto large = render (p, in, in, sr, static_cast<int> (n), 2);

    CHECK (maxAbsDifference (small.l, large.l) < 1.0e-6f,
           "output must not depend on how the host chops up the audio");
    CHECK (maxAbsDifference (small.r, large.r) < 1.0e-6f,
           "output must not depend on how the host chops up the audio (right)");
}

void testDeterminism()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 1.0);
    auto in = pinkNoise (n, 23u, 0.4f);

    Params p;
    p.failure = 80.0f;
    p.noise = NoiseLevel::High;
    p.spread = true;

    auto a = render (p, in, in, sr, 128, 2, 999u);
    auto b = render (p, in, in, sr, 128, 2, 999u);

    CHECK (maxAbsDifference (a.l, b.l) == 0.0f, "the same seed must give bit-identical output");
    CHECK (maxAbsDifference (a.r, b.r) == 0.0f, "the same seed must give bit-identical output (right)");

    auto c = render (p, in, in, sr, 128, 2, 1000u);
    CHECK (maxAbsDifference (a.l, c.l) > 0.0f, "a different seed must give different output");
}

void testSaturationLoudness()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 0.5);
    auto in = sine (n, 1000.0f, sr, 0.1259f);   // -18 dBFS

    Params p = quietParams();

    p.saturate = 0.0f;
    auto clean = render (p, in, in, sr, 128, 2);

    p.saturate = 100.0f;
    auto dirty = render (p, in, in, sr, 128, 2);

    const size_t skip = static_cast<size_t> (sr * 0.1);
    const float deltaDb = toDb (rms (dirty.l, skip)) - toDb (rms (clean.l, skip));

    CHECK (std::fabs (deltaDb) <= 2.0f,
           "SATURATE must change character, not level: RMS must stay within 2 dB");
}

void testAliasFloor()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 0.5);
    auto in = sine (n, 15000.0f, sr, 0.5f);

    Params p = quietParams();
    p.saturate = 100.0f;

    auto out = render (p, in, in, sr, 128, 2);

    const size_t skip = static_cast<size_t> (sr * 0.1);
    // The second harmonic of 15 kHz lands at 30 kHz and would fold back to
    // 18 kHz without oversampling.
    const float imageDb = toDb (goertzelAmplitude (out.l, 18000.0f, sr, skip));

    CHECK (imageDb < -55.0f, "the 2x oversampler must keep the aliased image below -55 dBFS");
}

void testModelSweepStability()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 2.0);
    auto in = sine (n, 440.0f, sr, 0.5f);

    Params p = quietParams();

    Core core;
    core.setSeed (0x5EEDBD42u);
    core.setParams (p);
    core.prepare (sr, 64, 2);

    std::vector<float> l = in, r = in;

    for (size_t pos = 0; pos < n; pos += 64)
    {
        const int count = static_cast<int> (std::min<size_t> (64, n - pos));
        p.model = 100.0f * static_cast<float> (pos) / static_cast<float> (n);
        core.setParams (p);

        float* ptrs[2] = { l.data() + pos, r.data() + pos };
        core.process (ptrs, 2, count);
    }

    CHECK (allFinite (l), "sweeping MODEL must not destabilise the filter cascade");
    CHECK (peak (l) < 4.0f, "sweeping MODEL must not produce a level excursion");
    CHECK (maxConsecutiveDelta (l) < 0.5f, "sweeping MODEL must not click");
}

void testFailureStatistics()
{
    constexpr double sr = 44100.0;
    const size_t n = static_cast<size_t> (sr * 60.0);

    Params p = quietParams();
    p.failure = 50.0f;

    Core core;
    core.setSeed (0x5EEDBD42u);
    core.setParams (p);
    core.prepare (sr, 512, 2);

    std::vector<float> l (512, 0.0f), r (512, 0.0f);
    auto chunk = pinkNoise (512, 31u, 0.3f);

    std::vector<float> collected;
    collected.reserve (n);

    for (size_t pos = 0; pos < n; pos += 512)
    {
        l = chunk;
        r = chunk;
        float* ptrs[2] = { l.data(), r.data() };
        core.setParams (p);
        core.process (ptrs, 2, 512);
        collected.insert (collected.end(), l.begin(), l.end());
    }

    // lambda = 0.02 + 0.5^2 * 6.0 = 1.52 events/sec per channel, two channels,
    // 60 seconds -> mean 182.4. The 99% interval is mean +/- 2.58 * sqrt(mean).
    const double mean = 2.0 * (0.02 + 0.25 * 6.0) * 60.0;
    const double halfWidth = 2.58 * std::sqrt (mean);
    const int count = core.failureEventCount();

    CHECK (static_cast<double> (count) > mean - halfWidth
             && static_cast<double> (count) < mean + halfWidth,
           "failure events must arrive at the expected Poisson rate");

    CHECK (allFinite (collected), "a minute of failures must stay finite");
    CHECK (maxConsecutiveDelta (collected) < 1.0f,
           "failure envelopes must not click");
}

void testMonoToStereo()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 2.0);
    auto in = pinkNoise (n, 37u, 0.4f);

    Params p = quietParams();
    p.wow = 60.0f;
    p.flutter = 40.0f;

    auto out = render (p, in, in, sr, 128, 2);

    const size_t skip = static_cast<size_t> (sr * 0.2);
    std::vector<float> l (out.l.begin() + static_cast<long> (skip), out.l.end());
    std::vector<float> r (out.r.begin() + static_cast<long> (skip), out.r.end());

    const float c = correlation (l, r);
    std::printf ("        (inter-channel correlation %.4f)\n", static_cast<double> (c));
    CHECK (c > 0.3f && c < 0.999f,
           "a mono input must be widened, but not decorrelated into mush");
}

void testNoAllocationsInProcess()
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 0.5);
    auto in = pinkNoise (n, 41u, 0.4f);

    Params p;
    p.failure = 90.0f;
    p.noise = NoiseLevel::High;
    p.spread = true;
    p.dryType = DryType::Processed;
    p.auxHeld = true;

    (void) render (p, in, in, sr, 128, 2, 0x5EEDBD42u, /* watchAllocations */ true);

    CHECK (! gAllocationDetected.load(), "process() must not allocate");
}

// ----------------------------------------------------------------------------

struct TestCase
{
    const char* name;
    void (*fn)();
};

const TestCase kTests[] = {
    { "prepare matrix",        testPrepareMatrix },
    { "NaN/Inf fuzz",          testNanFuzz },
    { "denormal guard",        testDenormalGuard },
    { "null transparency",     testNullTransparency },
    { "wet unity gain",        testWetUnityGain },
    { "latency",               testLatency },
    { "block-size invariance", testBlockSizeInvariance },
    { "determinism",           testDeterminism },
    { "saturation loudness",   testSaturationLoudness },
    { "alias floor",           testAliasFloor },
    { "MODEL sweep stability", testModelSweepStability },
    { "failure statistics",    testFailureStatistics },
    { "mono to stereo",        testMonoToStereo },
    { "no allocation",         testNoAllocationsInProcess },
};

// ----------------------------------------------------------------------------

int dumpWavs (const std::string& dir)
{
    constexpr double sr = 48000.0;
    const size_t n = static_cast<size_t> (sr * 6.0);

    struct Preset { const char* name; Params p; };

    auto makePreset = [] (float model, float wow, float flutter, float sat, float fail,
                          NoiseLevel noise, DryLevel dry)
    {
        Params p;
        p.model = model; p.wow = wow; p.flutter = flutter;
        p.saturate = sat; p.failure = fail;
        p.noise = noise; p.dryLevel = dry;
        return p;
    };

    const Preset presets[] = {
        { "clean",       makePreset (  0.f,  0.f,  0.f,   0.f,  0.f, NoiseLevel::Off,  DryLevel::None) },
        { "cassette",    makePreset ( 20.f, 20.f, 15.f,  30.f,  8.f, NoiseLevel::Low,  DryLevel::Small) },
        { "vhs",         makePreset ( 35.f, 35.f, 30.f,  45.f, 20.f, NoiseLevel::Low,  DryLevel::Small) },
        { "camcorder",   makePreset ( 55.f, 50.f, 45.f,  60.f, 35.f, NoiseLevel::High, DryLevel::None) },
        { "dictaphone",  makePreset ( 82.f, 60.f, 55.f,  75.f, 50.f, NoiseLevel::High, DryLevel::None) },
        { "destroyed",   makePreset (100.f, 90.f, 85.f, 100.f, 90.f, NoiseLevel::High, DryLevel::None) },
    };

    const struct { const char* name; std::vector<float> data; } sources[] = {
        { "drums", percussive (n, sr) },
        { "sweep", logSweep (n, 20.0f, 20000.0f, sr, 0.4f) },
    };

    int written = 0;
    for (const auto& src : sources)
    {
        for (const auto& preset : presets)
        {
            auto out = render (preset.p, src.data, src.data, sr, 256, 2);
            const std::string path = dir + "/bd-vhs_" + src.name + "_" + preset.name + ".wav";
            if (writeWav (path, { out.l, out.r }, sr))
            {
                std::printf ("  wrote %s\n", path.c_str());
                ++written;
            }
            else
            {
                std::printf ("  FAILED to write %s\n", path.c_str());
            }
        }
    }
    return written;
}

} // namespace

// ============================================================================

int main (int argc, char** argv)
{
    if (argc >= 3 && std::strcmp (argv[1], "--dump") == 0)
    {
        std::printf ("BD-VHS: rendering listening material into %s\n", argv[2]);
        const int written = dumpWavs (argv[2]);
        std::printf ("%d files written\n", written);
        return written > 0 ? 0 : 1;
    }

    std::printf ("BD-VHS core tests\n\n");

    const auto start = std::chrono::steady_clock::now();

    for (const auto& test : kTests)
    {
        gCurrentTest = test.name;
        const int before = gFailures;
        const auto t0 = std::chrono::steady_clock::now();

        test.fn();

        const double ms = std::chrono::duration<double, std::milli> (
                              std::chrono::steady_clock::now() - t0).count();
        std::printf ("  %-24s %s  (%.0f ms)\n",
                     test.name, (gFailures == before) ? "ok  " : "FAIL", ms);
    }

    const double totalMs = std::chrono::duration<double, std::milli> (
                               std::chrono::steady_clock::now() - start).count();

    std::printf ("\n%d checks, %d failures, %.0f ms\n", gChecks, gFailures, totalMs);
    return gFailures == 0 ? 0 : 1;
}
