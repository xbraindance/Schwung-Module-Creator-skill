# Native DSP Plugin (C/C++)

**When to load:** writing or debugging a native audio/MIDI plugin (`.so`), defining parameters, bridging runtime-discovered data between DSP and UI, or parsing MIDI files inside the DSP.

## Authoritative upstream
- `docs/MODULES.md` (Native DSP Plugin, JS↔DSP Communication, Chain Parameters)
  — https://github.com/charlesvestal/schwung/blob/main/docs/MODULES.md
- `docs/SPI_PROTOCOL.md` (audio buffer layout)
- Header: `src/host/plugin_api_v1.h`, `src/host/audio_fx_api_v2.h`, `src/host/midi_fx_api_v1.h`
- Reference DSPs to copy from: `src/modules/audio_fx/freeverb/freeverb.c`, `src/modules/sound_generators/linein/linein.c`

Optional private notes (may not exist on your machine): local
`schwung-wiki/patterns/dsp-patterns.md`, `schwung-wiki/gotchas/device-constraints.md`, BD-1200 notes.

## Audio format (fixed)

- 44 100 Hz, **128 frames per block** (~2.9 ms), stereo interleaved int16 little-endian: `[L0, R0, L1, R1, …, L127, R127]`
- Host master volume is applied **after** your `process()` / `render_block()` — don't pre-scale above int16 range
- No variable block sizes

## Pick the right API for your component type

There are **three** DSP APIs. Exporting the wrong entry symbol = silent load failure.

| `component_type` | Struct | Entry symbol |
|---|---|---|
| `sound_generator` | `plugin_api_v2_t` (api_version = 2) | `move_plugin_init_v2` |
| `audio_fx` | `audio_fx_api_v2_t` (api_version = 2) | `move_audio_fx_init_v2` |
| `midi_fx` | `midi_fx_api_v1_t` (api_version = 1) | `move_midi_fx_init` |

Two `api_version`s exist and they are independent:

- The module.json `api_version` field (optional, default 1 via `module_manager.c`) is a validation gate — the host accepts both 1 and 2 and rejects anything else. It does **not** select which init symbol the host probes.
- The DSP struct's `api_version` field is the ABI version of the returned struct, and must match the init symbol you exported.

Symbol selection rules:

- **Host-level sound generators** — `module_manager.c` always probes `move_plugin_init_v2` first via `dlsym`, then falls back to `move_plugin_init_v1` if v2 is absent or its `create_instance` fails.
- **Signal Chain slots** — the chain host (`src/modules/chain/dsp/chain_host.c`) picks the symbol from the slot's `component_type`: sound_generator ⇒ `move_plugin_init_v2`, audio_fx ⇒ `move_audio_fx_init_v2`, midi_fx ⇒ `move_midi_fx_init`.

So in practice, just match your DSP's exported entry symbol to your `component_type` (see the table above). The `api_version` field in module.json can usually be omitted; set it to `2` if you want to be explicit about the struct you return. Freeverb is a real example of an `api_version: 1` module.json whose DSP exports `move_audio_fx_init_v2` — it works because the chain audio_fx loader doesn't consult `api_version` at all.

### Sound generator / audio FX (v2)
```c
#include "host/plugin_api_v1.h"

static void render_block(void *inst, int16_t *out_lr, int frames) {
    /* 128 stereo frames */
}
static plugin_api_v2_t api = {
    .api_version = 2,
    .create_instance = create_instance,
    .destroy_instance = destroy_instance,
    .on_midi = on_midi,
    .set_param = set_param,
    .get_param = get_param,
    .render_block = render_block,
};
plugin_api_v2_t* move_plugin_init_v2(const host_api_v1_t *host) { return &api; }
```

### MIDI FX (v1 — different struct and signature)
```c
typedef struct midi_fx_api_v1 {
    uint32_t api_version;  /* must be 1 */
    void* (*create_instance)(const char *module_dir, const char *config_json);
    void  (*destroy_instance)(void *inst);
    int   (*process_midi)(void *inst, const uint8_t *in_msg, int in_len,
                          uint8_t out_msgs[][3], int out_lens[], int max_out);
    int   (*tick)(void *inst, int frames, int sample_rate,
                  uint8_t out_msgs[][3], int out_lens[], int max_out);
    void  (*set_param)(void *inst, const char *key, const char *val);
    int   (*get_param)(void *inst, const char *key, char *buf, int buf_len);
} midi_fx_api_v1_t;
midi_fx_api_v1_t* move_midi_fx_init(const host_api_v1_t *host);
```

`process_midi` handles one incoming message and emits up to `max_out` outgoing messages; `tick` emits unsolicited messages on a timer.

## Host pointer lifetime

The `host_api_v1_t *host` passed to `_init` (and forwarded to
`create_instance` at startup) points to a member of the long-lived
module-manager struct inside the host (`src/host/module_manager.c`
constructs `mm->host_api` once and passes `&mm->host_api` for the life
of the process). It is safe to store globally.

Convention in-repo:

```c
static const host_api_v1_t *g_host = NULL;

plugin_api_v2_t* move_plugin_init_v2(const host_api_v1_t *host) {
    g_host = host;   /* freeverb, linein, wav_player, chain_host, velocity_scale all do this */
    return &api;
}
```

Don't call `host->log` from inside `render_block` / `process_midi` — not
because the pointer is invalid, but because logging does fprintf
under the hood (see `references/realtime.md`).

## Parameter bridge — `set_param` / `get_param`

String-based; everything is `const char *`. JS calls `host_module_set_param(key, val)` and `host_module_get_param(key)`; those land in your two callbacks.

### The `parse_enum` trap
Don't write this:
```c
static int parse_enum(const char **names, int count, const char *val) {
    int v = find_enum(names, count, val);
    if (v >= 0) return v;
    v = atoi(val);                                 /* atoi("C01") → 0 */
    return (v >= 0 && v < count) ? v : 0;
}
```
Any custom string that happens to start with a non-digit falls back through `atoi()` to 0 — your "custom chord C01" silently becomes the first built-in. Use `find_enum` (returns -1 on miss) and then check extended lists explicitly.

## UI ↔ DSP for runtime-discovered data

The UI has no filesystem access. To expose counts/names of things the DSP discovered on disk:

DSP:
```c
if (strcmp(key, "custom_chord_count") == 0)
    return snprintf(buf, buf_len, "%d", inst->custom_chord_count);
if (strcmp(key, "custom_chord_names") == 0) {
    int pos = 0;
    for (int i = 0; i < inst->custom_chord_count; i++) {
        if (i > 0) buf[pos++] = ',';
        pos += snprintf(buf+pos, buf_len-pos, "%s", inst->custom_chords[i].name);
    }
    return pos;
}
```
UI:
```js
const count = parseInt(dspGet('custom_chord_count')) || 0;
const names = count ? dspGet('custom_chord_names').split(',') : [];
EDIT_ENUMS.chord_type = [...BUILTIN_CHORDS, ...names];
```

## Signal Chain metadata

Return from `get_param`:
- `"chain_params"` — JSON array of knob-mappable params. Keys **must** match `set_param`/`get_param` keys exactly.
- `"ui_hierarchy"` — nested menu tree for Shadow UI. May be stateful (hide items conditionally).

See `references/signal-chain.md` for exact shapes.

## File I/O from the DSP

`create_instance(module_dir, ...)` receives the absolute install path (e.g. `/data/UserData/schwung/modules/midi_fx/<id>/`). **Always** build paths relative to `module_dir`; never hardcode `/tmp` (rootfs is ~463 MB and almost always 100% full — writes crash the device).

```c
snprintf(inst->preset_dir, sizeof inst->preset_dir, "%s/presets/", module_dir);
```

Scan once at `create_instance` time — don't `opendir` per frame.

## MIDI file parsing essentials

If your module ingests `.mid` files:
- Variable-length delta times: each byte with MSB set continues; final byte has MSB clear
- Running status: data bytes < 0x80 reuse the previous status byte
- Note-on with velocity 0 == note-off (handle both 0x80-0x8F and 0x90-0x9F)
- Skip meta events (`0xFF`) and SysEx (`0xF0` / `0xF7`) by reading their length and jumping past
- Cap accepted file size at ~8 KB for chord/pattern files

## Preset save/load

Store enum values as **strings**, not indices — so custom values (e.g. user-loaded chords) survive round-trips even if the list shrinks. On load, look up by name; fall back to a sane default if the string is unknown.

## Realtime-path rules

These are load-bearing — see `references/realtime.md` for the full story:

- No `fprintf`, `fopen`, file I/O in `render_block` / `process_midi`
- No `malloc`/`new` in the hot path — pre-allocate in `create_instance`
- `host->log(msg)` takes a single `const char *` (not variadic). It wraps `fprintf`, so it's **not** realtime-safe — never call it from `render_block`. For variadic formatting outside the hot path, use the `LOG_DEBUG(src, fmt, …)` macro family from `src/host/unified_log.h`, or `snprintf` into a buffer first and then pass to `host->log`.

## Debug

- Verify entry symbol: `nm -D dsp.so | grep -E 'move_plugin_init_v2|move_audio_fx_init_v2|move_midi_fx_init'`
- DSP crashes silently on-device → enable logging (`references/deploy-debug.md`) and tail `/data/UserData/schwung/debug.log`
- Parameter mismatches: the UI sees the raw string; log it on the DSP side and compare
