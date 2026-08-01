# BD Clip — Soft/Hard Morphing Audio Clipper (Schwung module)

## Context

BD Clip is an audio clipper effect for Ableton Move, built as a Schwung module. Its signature feature is a continuous control that morphs the clipping character between soft (smooth/tanh saturation) and hard (brick-wall) clipping, plus the supporting parameters needed to make that musically useful. The module is named **`bd-clip`** and lives under a new `move/` directory — a staging area for real, deployable Move modules, ready to be copied into a Schwung checkout or pushed straight to a device.

Research grounding: the design is based on the real `audio_fx_api_v2_t` header and the full verbatim source of `freeverb.c` — the reference chainable `audio_fx` module — together with its `module.json`, pulled directly from the upstream Schwung repo, so it follows working, shipped code rather than second-hand summaries. A design-review pass then verified the ABI usage and the DSP math for edge cases.

## Module design

- **Location:** `move/bd-clip/`
- **id:** `bd-clip` · **name:** "BD Clip" · **abbrev:** "CLIP" · **version:** "0.1.0"
- **component_type:** `audio_fx`, **chainable:** `true` (nested inside `capabilities`, matching the only form the chain UI reads)
- **No standalone `ui.js`.** Verified via direct fetch that freeverb has no `ui.js` at all (only `module.json`, `freeverb.c`, `ui_chain.js`) — audio_fx modules only ever live inside a Signal Chain slot, which always loads `ui_chain.js`.
- **Processing is per-channel** (L and R independently) — explicitly *not* summed to mono like freeverb's reverb tank does; a clipper must keep the stereo image intact.

### Parameters (all `float`, stored 0.0–1.0, freeverb's percent-knob convention)

| key | knob? | meaning | range mapping | default |
|---|---|---|---|---|
| `drive` | yes (CC71) | pre-clip gain | 0%→0dB … 100%→+30dB | 0.2 |
| `shape` | yes (CC72) | **soft↔hard morph** — linear crossfade between `tanh` output and hard-clamp output | 0=fully soft, 1=fully hard | 0.3 |
| `mix` | yes (CC73) | dry/wet | 0=dry, 1=wet | 1.0 |
| `output` | yes (CC74) | post-clip trim | 0%→-24dB … 100%→+24dB (0.5=0dB) | 0.5 |
| `threshold` | no (menu-only, like freeverb's `width`) | clip ceiling as fraction of full scale | clamped internally to a 0.02 floor (divisor) | 1.0 |

Per-sample formula (verified realtime-safe: no divide-by-zero, no NaN/overflow at any parameter extreme, ~256 `tanhf` calls/block is a small fraction of the ~900µs budget — cheaper than freeverb's own comb/allpass buffer work, which runs fine on real hardware):

```
t       = max(threshold, 0.02)
driven  = in_sample_float * drive_lin        // in_sample_float = int16 / 32768.0f
x       = driven / t
soft    = tanhf(x)
hard    = clamp(x, -1, 1)
shaped  = soft*(1-shape) + hard*shape
clipped = shaped * t
wet     = clipped * output_lin
out     = in_sample_float*(1-mix) + wet*mix
out     = clamp(out, -1, 1)
sample_out = (int16_t)(out * 32767.0f)       // note: divide by 32768, multiply by 32767 — matches freeverb exactly, avoids an out-of-range int16 cast at the +1.0 boundary
```
`drive_lin` and `output_lin` are derived once per `set_param` call (`powf(10, db/20)`), never per-sample. Every param is defensively clamped to `[0,1]` in `set_param` regardless of what the caller sends (matches freeverb's own belt-and-suspenders convention), with `threshold` additionally floored at 0.02 since it's used as a divisor.

`chain_params` (the DSP-side knob map read by the chain host) will be implemented — freeverb omits it, but it's essentially free to add and is forward-compatible insurance for any host mechanism that reads it without loading our custom `ui_chain.js`. Scoped to just the 4 knob params, matching the `knobs` list.

`ui_hierarchy` is provided in two places, matching freeverb's actual pattern — both are needed, they serve different consumers:
- `module.json`'s `capabilities.ui_hierarchy`: the rich, static form (name/min/max/default/step/unit/display_format per param) — the source of truth for display metadata.
- DSP `get_param("ui_hierarchy")`: the simple runtime form (bare `knobs`/`params` key-name arrays) — feeds the host's generic Shadow UI overlay, which is why `threshold` is reachable even though it's not in `ui_chain.js`'s own quick-view.

`ui_chain.js` hardcodes its own small param table as JS constants rather than reading either JSON source at runtime (matches freeverb's actual behavior, simpler, no added parsing dependency).

## Critical files to create

**`move/bd-clip/module.json`** — structural clone of freeverb's module.json shape (id/name/abbrev/version/dsp/ui_chain/api_version/component_type + nested capabilities with chainable, component_type, and the full `ui_hierarchy` param table above). `api_version: 1` (matches freeverb precedent — it's a validation gate independent of which callbacks the DSP struct actually populates). No `ui` field, no `audio_in`/`audio_out` capability flags — freeverb (a real audio-in/audio-out module) declares neither, so this matches real precedent rather than adding speculative flags. Must stay valid per the minimal JSON parser (double-quoted keys/strings, lowercase booleans, no comments/trailing commas, ≤8KB).

**`move/bd-clip/src/bd_clip.c`** — native DSP plugin against `audio_fx_api_v2_t`. Structure (mirrors `freeverb.c` function-for-function):
- Includes: `<stdio.h> <stdlib.h> <string.h> <math.h>`, `"host/plugin_api_v1.h"`, `"host/audio_fx_api_v2.h"`.
- `bd_clip_instance_t`: raw params (`drive_raw, threshold_raw, shape_raw, mix_raw, output_raw`) + derived (`drive_lin, output_lin, thresh_safe`).
- `bd_clip_update_derived(inst)`: recomputes the 3 derived values from the 5 raw ones; called at end of `create_instance` and end of every `set_param`.
- `create_instance(module_dir, config_json)`: both args unused, `calloc` the instance, set the 5 defaults, call update_derived, return.
- `destroy_instance`: NULL check + `free`.
- `process_block(instance, audio_inout, frames)`: the per-sample formula above, applied independently to both interleaved channels.
- `set_param(instance, key, val)`: `"state"` key restores all 5 from a JSON blob (reuse freeverb's generic `json_get_float` helper); otherwise `atof` + clamp `[0,1]` into the matching field; always calls `update_derived`.
- `get_param(instance, key, buf, buf_len)`: per-param getters (`%.2f`), `"name"` → "BD Clip", `"state"` → JSON of all 5 raw values (`%.4f`), `"ui_hierarchy"` → the simple runtime form above, `"chain_params"` → JSON array of the 4 knob params (`type:"float", min:0, max:1, step:0.05`). Use freeverb's defensive `strlen`-checked-`strcpy` pattern — never an `strncpy` that can leave the buffer unterminated at the boundary.
- `move_audio_fx_init_v2(host)`: only non-static symbol in the file (everything else, including `g_host` and the static `audio_fx_api_v2_t` struct, must be `static` — Schwung `dlopen()`s multiple modules into one process, so unprefixed globals risk symbol collisions between modules). `memset` the struct, assign the 6 fields by name, leave `on_midi` as `NULL` (this module doesn't process MIDI — explicitly allowed by the header).
- No `malloc`/file I/O/`fprintf`/`host->log` inside `process_block` (realtime-safety rule) — logging only happens in `create_instance`/`destroy_instance`.

**`move/bd-clip/ui_chain.js`** — exports `globalThis.chain_ui = {init, tick, onMidiMessageInternal, onMidiMessageExternal}` (must not touch `globalThis.init`/`globalThis.tick`, which belong to the chain host). Imports `decodeDelta` (and ideally `shouldFilterMessage`) from `../../shared/input_filter.mjs`. Local `KNOB_PARAMS` table (drive/shape/mix/output, in that order, matching module.json's `knobs` array and CC71-74). `init()` pulls current values via `host_module_get_param`. `tick()` redraws only when a `needsRedraw` flag is set — a 4-line list, selected row inverted, "Name  NN%" per line, gated behind `clear_screen()`/`host_flush_display()`. `onMidiMessageInternal`: CC14 (jog) moves the selection cursor via `decodeDelta`; CC71-74 adjust their directly-mapped param (bypassing the cursor) via `decodeDelta` + a small fixed step, clamped to `[0,1]`, pushed with `host_module_set_param`. `onMidiMessageExternal` exported as a real no-op function rather than omitted (defensive — avoid a possible call-on-undefined if the chain host doesn't existence-check it).

## Verification

- `module.json`: validate with `python3 -c 'import json; json.load(open("move/bd-clip/module.json"))'` and confirm file size stays under 8KB.
- `bd_clip.c`: this sandbox has neither the real Schwung headers/checkout nor an ARM cross-toolchain, so a true build isn't possible here. As a lightweight sanity check, write throwaway local stub copies of `plugin_api_v1.h`/`audio_fx_api_v2.h` (matching the verbatim content already fetched from upstream) and host-compile with `gcc -c -Wall -Wextra` purely to catch syntax/type errors, then discard the stubs — they are not part of the deliverable. Manually re-check the file afterward against the realtime-safety rules (no `malloc`/file I/O/`fprintf` in `process_block`; every symbol but the entry point `static`).
- Manually trace 2-3 edge cases by hand (or a tiny host-side test harness compiled against the stubs): `shape=0` vs `shape=1` vs `shape=0.5` at high `drive`, `threshold` at its 0.02 floor — confirm output stays in range and behaves as designed.
- Real on-device confirmation (build via a Schwung checkout + `scripts/build.sh`/Docker, `scp`/`ssh` deploy, reboot for the `.so` to load, verify in the Signal Chain picker and via the display mirror) is out of reach from this sandbox and left to the user, but the plan above gives them a ready-to-drop-in, convention-correct module.

---

# Product Plan: Adaptive Signal Chain (Clipper + Transient Shaper)

This section extends the product beyond the single module deliverable above — it's the conceptual/product design for where this is headed, not tied to specific file paths.

## Signal chain

```
Synth/Track --> [ BD Clip ] --> [ Transient Shaper ] --> (rest of chain)
                 soft<->hard        attack/sustain
```

Placing the shaper *after* the clipper is deliberate: clipping (especially toward the `hard` end of the shape morph) reduces crest factor — it flattens exactly the transients a shaper acts on. Post-clip placement lets the shaper reclaim punch the clipping stage ate, or tame whatever spiky residue remains before the signal continues down the chain.

### Transient Shaper parameters (same knob-mappable convention as the clipper)

| key | knob? | meaning | range | default |
|---|---|---|---|---|
| `attack` | yes | boost/cut the initial transient | -100%..+100% | 0% |
| `sustain` | yes | boost/cut the body/tail | -100%..+100% | 0% |
| `mix` | yes | dry/wet | 0-100% | 100% |
| `output` | yes | trim | +-24dB | 0dB |

Detection: a fast/slow envelope-follower pair (their difference estimates the transient) — cheap, no FFT, same realtime budget class as the clipper.

## Making the product "intelligent"

Core idea: extract a handful of cheap features per block, map them to parameter targets — either as a one-shot "Suggest" action or (opt-in) continuous "Smart" adaptation.

**Realtime-feasible features (no FFT needed on embedded hardware):**
- RMS / peak / crest factor — how hot and how spiky the input already is
- Low/mid/high energy split via 2-3 one-pole filters — cheap, crude spectral balance
- Zero-crossing rate — cheap brightness/noisiness proxy (stand-in for spectral centroid)
- Transient density — how often the shaper's own fast/slow envelope-follower pair fires above a threshold — percussive vs sustained material

**Feature -> parameter mappings:**
- **Auto gain-staging:** expose a "target saturation amount" instead of a raw `drive` knob; solve for the `drive` value that hits that target given measured input RMS/peak, so quiet and hot sources produce the same *perceived* clip amount.
- **Shape bias from brightness:** bright/harsh input (high zero-crossing rate) nudges `shape` softer by default; dull/bass-heavy input can take a harder bias without becoming fatiguing.
- **Adaptive threshold:** track a running peak percentile instead of a fixed ceiling, so clipping engages relative to *this* material rather than an absolute level.
- **Shaper follows transient density:** percussive/high-density input leans `attack` positive (restore punch lost to clipping); sustained/low-density input (pads, bass) leans `sustain` positive instead.
- **Named "suggested outcomes":** a small on-device list (e.g. *Punchy*, *Warm*, *Loud/Safe*, *Lo-fi Crunch*, *Transparent*) — each label is a target vector over the params above; selecting one runs the analysis and solves toward that vector instead of the user hand-setting every knob.

**Interaction model (product decision, not just a technical one):**
- Default to a **one-shot "Suggest"** action (analyze -> snap to starting point -> user free to tweak) rather than silent continuous auto-adjustment, which risks feeling like the box is fighting the performer live. Offer continuous "Smart" mode as an explicit opt-in, smoothed over seconds so it never produces an audible jump.
- Any auto-adjustment needs on-screen feedback (what it detected, what it changed) or it just feels haunted.
- Heuristics (the hand-tunable feature->parameter function above) are realistic on Move's ARM budget today. A trained model (regression over tagged reference material) would tune the mapping better but isn't embedded-friendly — natural fit for a future desktop/DAW-plugin sibling, with learned coefficients ported back down to the hardware heuristic.

## UI mockups

128x64, 1-bit OLED. Real layout zones: header y0-11, content y12-52 @ 8px/line, footer y53-63, ~18-char safe width. `>` marks the jog-selected row (shown inverted — filled background, black text — on real hardware).

**0. Chain overview (Signal Chain slot strip)**
```
+--------------------------+
| SIGNAL CHAIN             |
|--------------------------|
|  Synth > [CLIP] > SHAPE  |
|              ^           |
|                          |
|                          |
|--------------------------|
| Jog:Move   Push:Edit     |
+--------------------------+
```

**1. BD Clip — expanded editor**
```
+--------------------------+
| BD Clip              1/2 |
|--------------------------|
|  Drive             20%  |
| >Shape              30% |
|  Mix               100% |
|  Output              0dB|
|--------------------------|
| Jog:Sel  K1-4:Adjust     |
+--------------------------+
```

**2. Transient Shaper — expanded editor**
```
+--------------------------+
| Transient Shaper      2/2|
|--------------------------|
| >Attack            +20% |
|  Sustain            -10%|
|  Mix               100% |
|  Output              0dB|
|--------------------------|
| Jog:Sel  K1-4:Adjust     |
+--------------------------+
```

**3. "Analyze" flow — triggered**
```
+--------------------------+
| BD Clip    ANALYZING     |
|--------------------------|
|                          |
|      [=====-----]        |
|                          |
|   Listening to input...  |
|                          |
|--------------------------|
| Back: Cancel             |
+--------------------------+
```

**4. Suggested-outcome result (one-shot "Suggest", not silent auto-adjust)**
```
+--------------------------+
| SUGGESTED SETTING        |
|--------------------------|
| Detected: Percussive,    |
|           Bright, Hot    |
|                          |
| Outcome:   >Punchy<      |
|--------------------------|
| Yes:Apply   No:Cancel    |
+--------------------------+
```

**5. Manual outcome picker (alternative entry point to the same targets)**
```
+--------------------------+
| SUGGEST OUTCOME       1/4|
|--------------------------|
| >Punchy                  |
|  Warm                    |
|  Loud / Safe             |
|  Lo-fi Crunch            |
|--------------------------|
| Jog:Select  Push:Go      |
+--------------------------+
```

## Open product questions

- Continuous "Smart" mode: always-available toggle, or gated behind a settings/menu flag so it can't surprise a performer mid-set?
- Should "Suggest" analyze a rolling window continuously in the background (ready the instant it's invoked) or only sample audio for a fixed window after being triggered?
- Outcome vocabulary (Punchy/Warm/Loud/Lo-fi/Transparent) is a starting proposal — worth validating against how the target user actually talks about tone before locking it in.

---
---

# BD Clip + Transient — JUCE Desktop Plugin Plan

*(Self-contained document. Intended destination: `/Users/click/Desktop/Move/BD-clip trans/`)*

## Context

The Move module is shaped by hardware limits: fixed 44.1 kHz, fixed 128-frame blocks, **int16** audio I/O, a ~900 µs ARM budget per 2.9 ms callback, no room for oversampling or FFT, four knobs and a 128×64 1-bit display. Every one of those forced a compromise on audio quality.

This plugin is the no-compromises sibling: **the same algorithm and the same sonic identity, engineered for maximum fidelity**, with the CPU/quality trade-off handed to the user as an explicit control rather than baked in by the hardware. The headline technical goal is eliminating the aliasing that a clipper unavoidably generates — which is precisely the thing the Move version cannot address at all.

## What the hardware forced, and what we reclaim

| Area | Move module | JUCE plugin |
|---|---|---|
| Sample format | int16 in/out | 32- or 64-bit float end-to-end |
| Sample rate | fixed 44.1 kHz | any host rate, with adaptive internal rate |
| Block size | fixed 128 frames | any, handled internally |
| Aliasing control | **none** (1×, no bandlimiting) | 1–16× oversampling **+ optional ADAA** |
| Parameter changes | instant, unsmoothed (accepted zipper risk) | per-sample smoothed |
| Lookahead | impossible (no latency budget) | opt-in, host-PDC compensated |
| Metering / display | 4 text lines, 1-bit | transfer curve, true-peak meters, spectrum |
| CPU headroom | ~900 µs/block, fixed | desktop-class, user-selectable |

## Signal flow

```
in ─▶ in-trim ─▶ ┌ OVERSAMPLED REGION ────────────────┐ ─▶ Transient Shaper ─▶ out-trim ─▶ mix ─▶ out
                 │ up ▶ drive ▶ thresh ▶ soft◀▶hard ▶ dn │    (detector + optional
                 └────────────────────────────────────┘     lookahead, gain mod)
```

The **clipper sits inside** the oversampled region — it is the nonlinearity, so it is the only stage that strictly needs it. The **shaper sits outside** by default: it is gain modulation, not waveshaping, and aliases far less. The top quality tier optionally oversamples the shaper's gain modulation at 2× as well.

Dry/wet mix is applied on a **latency-matched** dry path (delayed by the same amount as the wet chain), or the mix control silently smears phase.

## The core quality problem, stated precisely

Clipping is a memoryless nonlinearity: it generates harmonics with no bandlimiting, and everything above Nyquist folds back down as **inharmonic** aliasing — the metallic, "cheap digital" character. Hard clipping is the worst case (a discontinuous derivative produces a slowly-decaying harmonic series), which is exactly the end of the `shape` morph users will reach for when they want loud.

Two complementary mitigations, both worth shipping:

### 1. Oversampling
Run the nonlinearity at 2–16× the host rate so the generated harmonics land below the internal Nyquist, then filter and decimate. `juce::dsp::Oversampling` provides both filter families:
- `filterHalfBandPolyphaseIIR` — minimum phase, cheap, near-zero latency
- `filterHalfBandFIREquiripple` — linear phase, higher latency and CPU

Note its `factor` argument is the **number of 2× stages** (2 → 4×, 3 → 8×, 4 → 16×), and `useIntegerLatency = true` keeps host PDC clean.

### 2. ADAA (antiderivative anti-aliasing)
Dramatically reduces aliasing *without* raising the sample rate, so it pays off even in the low-CPU tiers. First-order form:

```
y[n] = ( F1(x[n]) − F1(x[n−1]) ) / ( x[n] − x[n−1] )
```

with a fallback to direct evaluation `f((x[n] + x[n−1]) / 2)` when `|x[n] − x[n−1]| < ε` — **this fallback is mandatory**, not optional; the expression is ill-conditioned as the denominator approaches zero and will produce loud garbage on near-DC input without it.

Closed-form antiderivatives for both of our curves:
- **Hard clip** `f(x) = clamp(x, −1, 1)` → `F1(x) = x²/2` for `|x| ≤ 1`, else `|x| − 1/2` (continuous at the knee: both give 0.5 at x=1)
- **tanh** → `F1(x) = ln(cosh(x))`, evaluated in the overflow-safe form `|x| + ln(1 + e^(−2|x|)) − ln(2)`

**The crossfade design is ADAA-friendly, and that is a real payoff of the original architecture.** Because the blend is linear in `shape` — `f_s(x) = (1−s)·tanh(x) + s·clip(x)` — and antidifferentiation is a linear operator, the blended antiderivative is simply `F1_s = (1−s)·F1_tanh + s·F1_clip`. No special-casing, no per-shape lookup tables.

**Subtlety a naive implementation will get wrong:** ADAA assumes the *same* transfer function is applied at sample `n` and `n−1`. With `shape` and `threshold` smoothed per-sample, that assumption breaks and produces small errors that show up as noise on fast knob moves. Fix: freeze `shape` and `threshold` across each sample pair (use the current sample's smoothed value for both `F1` evaluations), or hold them per-block on the ADAA path.

Known trade-off: ADAA introduces roughly a half-sample group delay and a slight HF softening. Acceptable, and it should be documented rather than hidden.

## Quality modes

Ship four presets over the underlying independent axes, with the axes also exposed for users who want them:

| Mode | Oversampling | Filter | ADAA | Precision | Lookahead | Latency |
|---|---|---|---|---|---|---|
| **Eco** | 1× | — | 1st order | float | off | zero |
| **Standard** (default) | 4× | polyphase IIR (min-phase) | 1st order | float | off | very low |
| **High** | 8× | FIR equiripple | 1st order | float | optional | moderate |
| **Ultra** | 16× | FIR equiripple (linear phase) | 2nd order | double | on | highest |

Two behaviours that matter more than the tiers themselves:

- **Auto-max on render.** Detect `isNonRealtime()` and force Ultra during offline bounce. Users get tracking-friendly CPU live and mastering-grade quality in the export, with no manual step to forget. This should be on by default.
- **Adaptive oversampling.** Target an *effective internal rate* (~352.8/384 kHz) rather than a fixed multiplier — at a 96 kHz session, 4× already exceeds what 8× buys at 44.1 kHz. Prevents users burning CPU for nothing at high session rates.

## Parameters (APVTS)

| Group | Param | Range | Default | Notes |
|---|---|---|---|---|
| Clipper | `drive` | 0…+30 dB | +6 dB | skewed; smoothed |
| | `threshold` | −40…0 dBFS | 0 dBFS | dB on desktop, same normalized value as Move internally |
| | `shape` | 0…100% (soft→hard) | 30% | the morph |
| | `clipMix` | 0…100% | 100% | |
| | `clipOutput` | ±24 dB | 0 dB | |
| Shaper | `attack` | −100…+100% | 0% | |
| | `sustain` | −100…+100% | 0% | |
| | `detectorHPF` | 20…500 Hz | 80 Hz | stops bass dominating detection |
| | `lookahead` | 0…5 ms | 0 ms | adds latency |
| | `shaperMix` | 0…100% | 100% | |
| Global | `qualityMode` | Eco/Std/High/Ultra | Standard | |
| | `osFactor`, `phaseMode`, `adaaOrder` | — | follow mode | advanced overrides |
| | `autoGain` | on/off | on | keeps perceived level constant vs drive |
| | `maxQualityOnRender` | on/off | **on** | |
| | `deltaListen` | on/off | off | audition only what the plugin adds |

Keep `threshold` displayed in dB for desktop musicality while storing the **identical normalized 0–1 value** the Move module uses — this is what makes preset parity possible later.

## Latency and PDC

`setLatencySamples(oversamplingLatency + lookaheadSamples)`, reported accurately or hosts will misalign the track.

The real design decision: **changing oversampling factor mid-playback changes latency**, and host tolerance for that varies from seamless to an audible re-sync glitch. Recommended: offer a **"constant latency"** option that pads every mode to the maximum, so quality switching is glitch-free and automatable — at the cost of always carrying Ultra's latency. Default it off (lowest latency), and document the trade.

## Shared DSP core with the Move module

Factor the algorithm into a **header-only, templated core** (`bd_clip_core.h`, `bd_shaper_core.h`) consumed by both targets:

- **Move** instantiates `float`, 1×, ADAA off (CPU-bound), int16 conversion at the edges.
- **JUCE** instantiates `float`/`double`, N×, ADAA on.

This is what guarantees the two products actually *sound like each other* rather than drifting into two different effects that share a name, and it lets the same unit tests validate both. Keeping the normalized parameter space and state keys identical across the two makes preset interchange a later config decision rather than a rewrite.

## Project structure

CMake + JUCE 9.0.0 (released 21 July 2026 — very new; pin the latest 8.0.x instead if proven stability matters more than the new SVG/font and CoreAudio work). Modules: `juce_dsp`, `juce_audio_processors`, `juce_audio_utils`. Formats: VST3, AU, Standalone (AAX only if Pro Tools is a target — it requires PACE signing and an Avid agreement).

```
BD-clip-trans/
├─ CMakeLists.txt
├─ core/                    ← shared with the Move module
│  ├─ bd_clip_core.h        (drive/threshold/soft↔hard morph + ADAA)
│  └─ bd_shaper_core.h      (dual envelope detector + gain mod)
├─ src/
│  ├─ PluginProcessor.{h,cpp}   (APVTS, oversampling, latency, quality modes)
│  ├─ PluginEditor.{h,cpp}
│  ├─ QualityManager.{h,cpp}    (mode → OS factor/filter/ADAA/precision)
│  └─ ui/  (TransferCurve, Meters, TransientDisplay)
└─ tests/  (aliasing, null, latency, core-parity vs Move)
```

## UI

The desktop screen removes the 128×64 ceiling entirely, and the single highest-value addition is a **live transfer-curve display** — the soft↔hard morph becomes something the user *sees* bending from a smooth tanh knee into a brick wall as they turn it. That one visual explains the whole product better than any label. Plus: input/output meters with true-peak, a transient gain-history strip, an optional spectrum view (which can show the aliasing floor dropping as quality increases — a persuasive demo of what the quality modes actually buy), A/B compare, and preset management. Vector-drawn and resizable.

## Testing and verification

- **Aliasing measurement** — the primary quality metric. Feed sine tones (1 kHz and a deliberately awkward 11 kHz) at high drive, FFT the output, measure inharmonic energy against the 1×/no-ADAA baseline. Expect a large improvement at Ultra; this number is the plan's success criterion, not a vibe check.
- **Null test** — at `mix = 0` the output must null against the dry input to within float epsilon. Catches latency-compensation and gain-staging bugs, which are the most common silent regressions here.
- **Latency test** — impulse in, measure actual delay, assert it equals the reported `getLatencySamples()` in every quality mode.
- **Core parity test** — the shared core, at 1× with ADAA off, must match the Move module's output on identical input vectors within tolerance.
- **Denormals** — `ScopedNoDenormals` in `processBlock`; verify no CPU spike as signals decay to silence.
- **pluginval** at strictness 10, plus a CPU benchmark per quality mode.

## Connection back to the "intelligent" features

The analysis/suggest work sketched in the product plan gets substantially easier here: desktop CPU allows real FFT-based feature extraction (true spectral centroid rather than the zero-crossing proxy), offline analysis of an entire selection instead of a live window, and a genuinely trainable model. The natural division of labour is to develop and tune the feature→parameter mapping on desktop where iteration is fast, then port the learned coefficients down to the Move module's cheap heuristic version.

## Open questions

- Is Pro Tools/AAX in scope? It adds PACE signing and an Avid agreement, so it changes project setup materially.
- Should presets be interchangeable with the Move module in both directions, or is the shared core enough (sonic parity without file compatibility)?
- Is a Standalone build with its own audio device I/O useful, or is this plugin-only?
