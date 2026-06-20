---
name: Schwung Module Creator
description: Build and troubleshoot Schwung modules for Ableton Move hardware. Use this skill whenever the user mentions Schwung, Move modules, SMC, module.json, ui.js, ui_chain.js, native DSP plugins, Signal Chain, chain_params, deploying to move.local, SSH into an Ableton Move, or debugging the Move display/logs — even if they don't explicitly say "use the skill".
codeInterpreter: false
---

# Schwung Module Creator (SMC)

Build and troubleshoot **Schwung modules** for Ableton Move. Dev loop: write → build → deploy → observe → fix → repeat.

## Working protocol

1. **Pick the integration area(s)** from the table below — most module tasks span 2-3.
2. **Read the matching `references/*.md`** file(s) before writing code. They are small on purpose.
3. Each reference points to the upstream authoritative files (headers,
   `docs/*.md`, reference modules). When in doubt, read the source in
   the Schwung repo at https://github.com/charlesvestal/schwung rather
   than trusting summaries.
4. Optional private notes (local `schwung-wiki/`, `BD-1200/` trees) may
   be referenced in places — they are supplementary and may not exist
   on your machine. Skip them if absent.

## Task → Reference map

| What you're doing | Load |
|---|---|
| New module skeleton, `module.json`, capabilities, component_type, silent load failures | [references/manifest.md](references/manifest.md) |
| Display (128×64), menus, encoders, `ui.js`, `ui_chain.js`, Back button, Shadow UI | [references/ui.md](references/ui.md) |
| Native DSP plugin (`.so`), `process()`, parameter hierarchy, chain_params DSP side | [references/dsp.md](references/dsp.md) |
| MIDI from pads/knobs/USB, cable-0 vs cable-2, echo filter, MIDI injection | [references/midi.md](references/midi.md) |
| Chainable modules, Signal Chain UI shims, MIDI FX chaining, `chain_params` wiring | [references/signal-chain.md](references/signal-chain.md) |
| `host_*` functions, file I/O, module lifecycle, sampler, TTS, system commands | [references/host-api.md](references/host-api.md) |
| Audio dropouts, SPI budget, FIFO priority, CPU pinning, child-process scheduling | [references/realtime.md](references/realtime.md) |
| Build, deploy, SSH, logs, display mirror, cache invalidation, reboot-vs-rescan; **Schwung Manager install, "No module.json found in tarball", Module Store catalog submission, release.json** | [references/deploy-debug.md](references/deploy-debug.md) |
| LD_PRELOAD shim, SPI mailbox, struct layout, fork hygiene (**only if forking Schwung**) | [references/architecture.md](references/architecture.md) |

## Knowledge base

### Primary (always available)
- Upstream repo: https://github.com/charlesvestal/schwung
  - `docs/MODULES.md`, `docs/API.md`, `docs/SPI_PROTOCOL.md`,
    `docs/LOGGING.md`, `docs/REALTIME_SAFETY.md`, `docs/MIDI_INJECTION.md`
  - `CLAUDE.md` at repo root — host + module architecture summary
  - `src/host/` headers for ABI contracts
  - `src/modules/` for reference implementations

### Optional private notes (may not be present on your machine)
- Local `schwung-wiki/` — framework/patterns/gotchas/modules/features
- Local `BD-1200/bd-1200-wiki/` — deep SP-1200 emulation notes

These were created by one contributor and aren't checked in anywhere
public. If a reference file points into them and you don't have them,
go to the upstream source file instead.

## Prerequisites

Claude Code · Ableton Move reachable on network · SSH configured (`ssh ableton@move.local`) · Docker or aarch64 cross-toolchain for builds · Chrome extension optional for display mirror at `http://move.local:7681`.

## Universal checklist (applies to every module)

- [ ] `module.json` is valid JSON — no comments, double-quoted keys, lowercase booleans, no trailing commas, ≤8 KB
- [ ] Correct `component_type` (drives install path + DSP entry symbol)
- [ ] `api_version: 2` if your DSP exports the v2 entry symbols; `1` (or omitted) if v1
- [ ] If chainable: `chainable: true` inside `capabilities` (top-level `chainable` is not read)
- [ ] `ui.js` exports `init`, `tick`, `onMidiMessageInternal`, `onMidiMessageExternal`
- [ ] `shouldFilterMessage()` called at top of `onMidiMessageInternal` (unless `raw_midi: true`)
- [ ] Display cleared in `init()`; Back button (CC 51) handled
- [ ] All file I/O under `/data/UserData/` — never `/tmp`
- [ ] DSP `render_block` / `process_midi` has no `fprintf`, no `host->log`, no allocation, no file I/O
- [ ] Tool modules call `host_exit_module()` on Back (non-tool modules must not call it)
- [ ] Overtake modules init LEDs progressively (≤8 LEDs/frame)

## Quick commands

```bash
# Build (Docker)
docker build -t my-module-builder -f scripts/Dockerfile . && \
  docker run --rm -v "$PWD:/build" -w /build my-module-builder ./scripts/build.sh

# Build (local macOS)
CROSS_PREFIX=aarch64-unknown-linux-gnu- ./scripts/build.sh

# Deploy
scp dist/my-module-module.tar.gz ableton@move.local:/data/UserData/
ssh ableton@move.local 'tar -xzf /data/UserData/my-module-module.tar.gz \
  -C /data/UserData/schwung/modules/audio_fx/'

# Reboot (DSP/native changes only)
ssh root@move.local 'reboot'

# Enable logs + tail
ssh ableton@move.local 'touch /data/UserData/schwung/debug_log_on'
ssh ableton@move.local 'tail -f /data/UserData/schwung/debug.log'

# Display mirror / Move Manager
open http://move.local:7681
open http://move.local:7700
```

## External links

- Schwung upstream: https://github.com/charlesvestal/schwung
- Upstream docs: https://github.com/charlesvestal/schwung/tree/main/docs
- This skill repo: https://github.com/xbraindance/Schwung-Module-Creator-skill
