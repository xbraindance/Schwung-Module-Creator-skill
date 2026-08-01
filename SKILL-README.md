# Schwung Module Creator Skill

Build **Schwung modules** for Ableton Move hardware with Claude Code. Progressive-disclosure skill with focused references for each integration area: architecture, DSP APIs, UI, MIDI, signal chain, realtime safety, and deployment.

## Quick Start

Invoke the skill in Claude Code:

```
Use the Schwung Module Creator skill to help me create a new module called "my-synth"
```

The skill will route you to the right reference based on your task — architecture decisions, DSP implementation, UI patterns, signal chain setup, or troubleshooting.

## Architecture

```
skill.md                      # Lean router (99 lines)
                              # → Task-to-reference mapping table
                              # → Universal checklist
                              # → Quick command reference

references/
├── manifest.md              # module.json syntax, component_type, capabilities
├── dsp.md                   # Three DSP APIs, audio format, parameter bridge
├── ui.md                    # Display zones, MIDI CC layout, ui_chain.js
├── midi.md                  # Cable routing, host transforms, injection safety
├── host-api.md              # Filesystem rule, module mgmt, lifecycle
├── signal-chain.md          # chainable flag, chain_params, ui_hierarchy
├── realtime.md              # SPI budget, forbidden calls, scheduling
├── deploy-debug.md          # Build, deploy, logs, common failures
└── architecture.md          # Bootstrap layers, SPI mailbox, struct safety
```

**Philosophy:** Wikis are authoritative. References point to them. Only production-tested gotchas stay inline. Each reference loads on-demand — you only see what matters for your current task.

## What Each Reference Covers

| Reference | Use When | Key Content |
|---|---|---|
| **manifest.md** | Starting a new module | `module.json` structure, 8 KB minimal parser rules, `component_type` → install path mapping, capabilities flags |
| **dsp.md** | Writing DSP code | Three API structs (sound_generator, audio_fx, midi_fx), audio format, enum handling, parameter bridge, preset save/load, file I/O |
| **ui.md** | Building UI | Display zones (header/content/footer), full MIDI CC hardware map, common UI bugs + fixes, ui_chain.js pattern |
| **midi.md** | MIDI handling | Cable routing, velocity curves, aftertouch, knob delta decoding, injection safety, rate limits |
| **host-api.md** | Calling host from JS | Filesystem rule (use `/data/UserData/` only), module management, parameter bridge, lifecycle |
| **signal-chain.md** | Making module chainable | `chainable` flag both places, `chain_params` JSON array, `ui_hierarchy`, ui_chain.js for chain context |
| **realtime.md** | Audio dropouts or glitches | SPI budget (~900 µs), forbidden calls, scheduling, CPU pinning, MIDI/LED budgets |
| **deploy-debug.md** | Building & deploying | Docker build, SSH deploy, log enable/tail, verify checklist, common failures |
| **architecture.md** | Forking or understanding bootstrap | Four bootstrap layers, SPI mailbox, struct layout safety, Link Audio routing |

## Common Workflows

### Create a new audio FX module

1. Read **manifest.md** → `component_type: "audio_fx"` → install path `.../audio_fx/`
2. Read **dsp.md** → `audio_fx_api_v2_t` struct, render loop, parameter bridge
3. Read **ui.md** → display zones, MIDI CC knobs, menu pattern
4. Deploy via **deploy-debug.md** → build, SSH, verify exports

### Debug audio dropouts

1. Read **realtime.md** → SPI budget check, forbidden calls
2. Enable logs via **deploy-debug.md**
3. Check `top -H`, verify child process scheduling via **realtime.md**

### Make module chainable

1. Read **manifest.md** → `chainable: true` in two places
2. Read **signal-chain.md** → `chain_params` JSON, `ui_hierarchy`
3. Read **dsp.md** → parameter bridge for runtime data
4. Create `ui_chain.js` per **signal-chain.md**

## Key Gotchas

- **`module.json` syntax**: Minimal JSON parser — no comments, double quotes only, lowercase `true`/`false`, 8 KB cap
- **`chainable` flag**: Must appear in both top-level AND capabilities object in `module.json` or module silently won't appear in chain picker
- **Filesystem**: `/` is ~463 MB and almost always 100% full. Always write to `/data/UserData/`. Never use `/tmp`.
- **SPI budget**: ~900 µs per 2.9 ms block. No file I/O, malloc, or logging inside `render_block()` or MIDI callback.
- **DSP exit symbols**: Different for each `component_type` (`move_audio_fx_init_v2`, `move_midi_fx_init`, etc.). Check **manifest.md** table.

## Prerequisites

- Claude Code (web, desktop, or IDE extension)
- Ableton Move device on your network (optional, for deploy/test)
- SSH access for remote build/deploy: `ssh ableton@move.local`
- Schwung installed on Move (see [schwung-main/docs/INSTALLATION.md](https://github.com/charlesvestal/schwung/blob/main/docs/INSTALLATION.md))

## Resources

- **Authoritative Schwung Docs**: [schwung-main/docs/](https://github.com/charlesvestal/schwung/tree/main/docs) — ARCHITECTURE.md, API.md, MODULES.md
- **Schwung Wiki**: [schwung-wiki/](https://github.com/charlesvestal/schwung/tree/main/schwung-wiki) — Deploy patterns, gotchas, feature write-ups
- **BD-1200 Example**: [BD-1200/bd-1200-wiki/](https://github.com/charlesvestal/move-anything/tree/main/BD-1200/bd-1200-wiki) — 10-step rendering pipeline, signal chain reference implementation
- **Official Schwung Repo**: https://github.com/charlesvestal/schwung
- **Move Device Web**: `http://move.local` (web UI)
- **Display Mirror**: `http://move.local:7681` (live OLED view)

## Version

- **Skill Version**: 0.1.0
- **Schwung Compatibility**: v0.1.0+

---

**Built for Schwung module developers**
