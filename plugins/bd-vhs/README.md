# BD-VHS

Tape and video-cassette degradation, as a VST3 / CLAP / AU plugin.

Six controls that matter — **Wow**, **Flutter**, **Model**, **Saturate**,
**Failure**, **Volume** — plus three switches and a handful of hidden options,
wrapped around a signal path that behaves like a machine slowly failing rather
than like an EQ with a chorus after it.

> BD-VHS is an original effect inspired by the sound of tape and VCR
> degradation. It is not affiliated with, endorsed by, or derived from any pedal
> manufacturer's product. The machine profiles are designed by ear from the
> characteristic response of each class of recording machine, not measured from
> any specific product.

## Building

The project splits into a framework-free DSP core and a thin JUCE wrapper. The
core needs nothing but a C++17 compiler, so you can work on the sound without
installing a single system dependency:

```sh
cmake -S plugins/bd-vhs -B build-core -G Ninja -DBDVHS_BUILD_CORE_ONLY=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

The full build fetches JUCE 8.0.15 and clap-juce-extensions, and produces VST3
and CLAP everywhere plus AU on macOS:

```sh
cmake -S plugins/bd-vhs -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Artefacts land in `build/plugin/BDVHS_artefacts/Release/`.

On Linux the JUCE targets need X11 development headers:

```sh
sudo apt-get install -y libx11-dev libxcomposite-dev libxcursor-dev \
    libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
    libfreetype-dev libfontconfig1-dev
```

If those are unavailable, configure with `-DBDVHS_BUILD_CORE_ONLY=ON` and let CI
compile the wrapper. `-DBDVHS_MINIMAL_X11=ON` drops the optional X11 extensions
if you want a leaner link.

### Options

| Option | Default | Effect |
|---|---|---|
| `BDVHS_BUILD_CORE_ONLY` | `OFF` | Skip every JUCE target. No system dependencies at all. |
| `BDVHS_BUILD_TESTS` | `ON` | Build the headless core test suite. |
| `BDVHS_BUILD_STANDALONE` | `OFF` | Build the standalone app. Needs ALSA on Linux. |
| `BDVHS_COPY_AFTER_BUILD` | `ON` | Install into the user plugin folders after building. |
| `BDVHS_MINIMAL_X11` | `OFF` | Disable optional JUCE X11 extensions. |
| `BDVHS_JUCE_TAG` | `8.0.15` | JUCE version to fetch. |
| `BDVHS_CLAP_EXT_REF` | pinned commit | clap-juce-extensions revision. |

### Listening material

```sh
./build-core/tests/bd_vhs_tests --dump /some/directory
```

Renders a grid of presets across a drum loop and a log sweep, as 32-bit float
WAVs.

## The controls

| Control | What it does |
|---|---|
| **Wow** | Slow, smooth, random pitch drift — up to ±7 ms of delay deviation at around 0.5 Hz. |
| **Flutter** | Fast, twitchy wobble around 8–12 Hz, pitch *and* amplitude. The amplitude component scales with the square of the knob, so low settings are pure pitch. |
| **Model** | Morphs continuously through eight machine profiles: Studio → Cassette → VHS SP → VHS EP → Camcorder → Answer Machine → Dictaphone → Toy. Each is a bandwidth, a head bump, a tilt and a presence character. |
| **Saturate** | Magnetic saturation. Pre/de-emphasis around the nonlinearity squashes the top end the way tape does, rather than the way a transistor does. |
| **Failure** | Dropouts, pitch snags, wrinkles and crackle, arriving as a Poisson process whose rate and severity both rise with the knob. |
| **Volume** | Output level. |
| **Mix** | Global wet/dry. Not on the original concept, but hosts and users expect one. At 0 the plugin is bit-transparent. |

| Switch | Positions |
|---|---|
| **Aux Mode** | Stop (tape stop) / Filter (lift the bandwidth limit) / Fail (force Failure to maximum) — fired by the **Aux Footswitch** parameter. |
| **Dry** | None / Small (−12 dB) / Unity. |
| **Noise** | Off / Low (−72 dBFS) / High (−54 dBFS). Band-limited by the current Model, which is what makes it sound like one machine rather than two things stacked. |

Hidden options, exposed as non-automatable parameters: **Noise Response**
(Static / Gated / Ducked), **Dry Type** (Clean / Processed — Processed runs the
dry blend through everything except wow and flutter), **Spread** (lets Failure
destabilise the stereo image), **Model Snap** (step instead of morph), and
**Ramp Mode** (Off / Ramp / Bounce).

## Behaviour worth knowing

**Latency.** The plugin reports **15 samples**, which is the saturation stage's
2× oversampler. That figure is a compile-time constant at every sample rate and
never changes at runtime.

The tape path's ~16 ms record-to-playback gap is deliberately *not* reported.
It applies to the wet signal only — the dry blend is aligned to the input — so
nothing inside the plugin is misaligned, and reporting it would make the dry
path lead the beat by 16 ms in every host. If you are doing sample-accurate
parallel processing against an external dry track, that 16 ms is real and
intended.

**Ramp and Bounce** are a documented simplification. Assigning ramp destinations
per-parameter needs a UI, so for now Ramp sweeps Wow, Flutter and Failure toward
maximum over Ramp Time while the footswitch is held, and Bounce oscillates them.
When Ramp Mode is not Off, it takes over the footswitch from the Aux effect.

**Spread** adds about 1 ms of internal delay to the wet path while engaged. It
is a configuration switch rather than a performance control, so this is not
smoothed across the toggle.

**Determinism.** Given a seed, the noise and the entire sequence of failure
events are reproducible bit-for-bit. Two renders of the same material with the
same settings are identical.

## Tests

```sh
ctest --test-dir build-core --output-on-failure
```

Fourteen tests, no framework — every assertion is numeric, so a test library
would cost a network dependency and a slower build without buying anything.
Notable ones:

- **Block-size invariance.** Ten seconds rendered in 64-sample blocks matches
  one 441,000-sample block to within 1e-6. This is what enforces the internal
  32-sample control grid being measured from the start of the stream rather than
  the start of each host block.
- **Null transparency.** At `mix = 0` the output is the input delayed by exactly
  the reported latency, bit-for-bit.
- **No allocation.** A global `operator new` override asserts that `process()`
  never allocates.
- **Alias floor.** A 15 kHz tone at full saturation keeps its folded image below
  −55 dBFS.
- **Failure statistics.** Event counts over a minute fall inside the 99 %
  Poisson interval for the configured rate.

## Architecture

```
core/     framework-free C++17. All the DSP. MIT licensed.
plugin/   JUCE AudioProcessor. Parameter marshalling and state only. AGPLv3.
tests/    dependency-free test suite, links core only.
tools/    offline profile-fitting helper.
```

The split is not decoration. The JUCE plugin target transitively requires
`juce_gui_basics` and therefore X11 development headers, which minimal build
images do not have. Keeping every algorithm in a library with no dependencies
means the sound can be developed and verified anywhere, and the wrapper — which
contains no algorithmic content — can be compile-checked on CI.

Adding a custom editor later is a pure addition: write the editor class, add it
to `target_sources`, flip `hasEditor()`, return it from `createEditor()`.

## Licensing

`core/` is MIT. `plugin/` is AGPLv3, because it links JUCE 8, which is
dual-licensed AGPLv3-or-commercial, and BD-VHS disables the JUCE splash screen —
which is only permitted under the AGPL or a paid JUCE licence. See `LICENSE`;
note it needs the full AGPL text appended before distribution.
