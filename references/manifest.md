# Module Manifest & Capabilities

**When to load:** creating a new module skeleton, editing `module.json`, choosing `component_type`, adding capabilities, or debugging silent load failures ("my module doesn't appear in the picker").

## Authoritative upstream
- `docs/MODULES.md` → Module Structure, Capabilities, Tool Config, Defaults
  — https://github.com/charlesvestal/schwung/blob/main/docs/MODULES.md
- `CLAUDE.md` (repo root) — component types, signal chain integration
- Host-side parser: `src/host/module_manager.c`

Optional private notes (may not exist on your machine): local
`schwung-wiki/framework/module-json-rules.md`, `schwung-wiki/gotchas/module-loading-failures.md`.

## JSON parser constraints (the minimal parser is picky)

- Double-quoted keys and strings only — no single quotes, no unquoted keys
- Lowercase booleans (`true`/`false`)
- No comments, no trailing commas
- No nested arrays inside `chain_params` (return them from DSP `get_param("chain_params")` instead — see `references/signal-chain.md`)
- 8 KB cap on the whole file

Validate locally: `python3 -c 'import json; json.load(open("module.json"))'`

## Required fields

| Field | Notes |
|---|---|
| `id` | Directory-safe; must match the installed folder name |
| `name` | Shown in picker |
| `version` | SemVer |
| `component_type` | Drives install-path subdirectory and, for chainable modules, which DSP entry symbol the Signal Chain looks up. Without it, the module won't categorize correctly in the menu. |

Optional but recommended:

| Field | Notes |
|---|---|
| `api_version` | Defaults to `1` if omitted. Use `2` for native DSP plugins that want multi-instance (`create_instance`/`destroy_instance`). The module.json field is separate from the DSP struct's `api_version` — see `references/dsp.md`. |
| `abbrev` | 3-6 chars, shown in Signal Chain slot view |
| `builtin` | Only the chain UI reads this, defaulting to `false`. Safe to omit for user-installed modules. |
| `description`, `author`, `ui`, `ui_chain`, `dsp`, `defaults`, `assets`, `scan_packs`, `requires_path` | See upstream `docs/MODULES.md` |

## Component type → install path + DSP API

Install path is derived from `component_type` — mismatch = module invisible in picker.

| `component_type` | Install path under `/data/UserData/schwung/modules/` | DSP entry symbol | DSP struct |
|---|---|---|---|
| `sound_generator` | `sound_generators/<id>/` | `move_plugin_init_v2` | `plugin_api_v2_t` |
| `audio_fx` | `audio_fx/<id>/` | `move_audio_fx_init_v2` | `audio_fx_api_v2_t` |
| `midi_fx` | `midi_fx/<id>/` | `move_midi_fx_init` | `midi_fx_api_v1_t` |
| `utility` | `utilities/<id>/` | (module-specific) | — |
| `tool` | `tools/<id>/` | — | (UI-only unless needed) |
| `overtake` | `overtake/<id>/` | — | UI-only, full display control |

**Trap:** exporting the wrong DSP entry symbol = silent load failure. See `references/dsp.md`.

## Capabilities

Declared inside `"capabilities": { ... }`:

| Flag | Effect |
|---|---|
| `audio_out` | Module produces audio; host routes through master volume |
| `audio_in` | Module reads input audio |
| `midi_in` | `onMidiMessageInternal/External` receive events |
| `midi_out` | Module emits MIDI (subject to echo filter if MIDI FX) |
| `aftertouch` | Pressure events forwarded |
| `claims_master_knob` | Volume knob automation disabled for this module; CC 79 goes to the module instead |
| `raw_midi` | Bypass **all** host transforms (velocity curve, aftertouch deadzone, knob-touch filter for notes 0-9). Not selective. |
| `raw_ui` | Module owns the Back button; you must call `host_return_to_menu()` yourself |
| `chainable` | Module appears in Signal Chain. Declared **inside `capabilities`** — a top-level `chainable` field is ignored by the chain UI. |
| `skip_led_clear` | Host does not clear LEDs on load/unload (preserve Move colors) |

## Assets (for modules with user-uploadable files)

Modules that scan a folder at load time (custom chords, samples, presets) declare it:

```json
"assets": {
  "path": "custom_chords",
  "label": "Custom Chords",
  "extensions": [".mid"],
  "optional": true,
  "description": "Drop .mid files here for additional chord definitions"
}
```

The declaration is **informational** — the DSP still scans the directory via `opendir/readdir`. See `references/dsp.md` "File I/O" for the scan pattern.

## Tool module extras

`tool_config` controls tool-picker behaviour:

```json
"tool_config": {
  "interactive": true,
  "skip_file_browser": true,
  "input_extensions": ["wav"],
  "command": "..."
}
```

Tool modules must call `host_exit_module()` on Back — calling it from any non-tool module crashes the host.

## Silent-failure triage

When a module doesn't show up in the picker:

1. JSON syntax invalid (comments? trailing commas? single quotes?) — validate with Python
2. `component_type` doesn't match the actual install path
3. `chainable` declared at top level instead of inside `capabilities` (only the nested form is read)
4. DSP `.so` present but exports the wrong entry symbol for this `component_type` (`nm -D dsp.so | grep init`)
5. `api_version: 2` declared but the DSP doesn't actually implement `create_instance`/`destroy_instance`
6. `module.json` > 8 KB
