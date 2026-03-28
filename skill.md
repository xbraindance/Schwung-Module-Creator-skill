---
name: schwung-module
description: >
  Use this skill whenever the user wants to create, scaffold, or develop a module for Schwung —
  the custom JavaScript and DSP framework for Ableton Move hardware. Trigger for any request involving
  writing a new module, adding a UI to Move, creating a synth/effect/MIDI tool/utility for Schwung,
  designing a Signal Chain component, writing a module.json, building a ui.js, or creating a dsp.so plugin.
  Also use when the user asks about module structure, how to handle pads/knobs/display, how to integrate with
  Signal Chain, or how to deploy/install a module to the Move device. This skill ensures you follow all
  architectural conventions, file naming rules, hardware constraints, and API patterns from the project.
---

# Schwung Module Creator

You are helping create a module for **Schwung** — a framework that extends Ableton Move hardware
with custom JavaScript UIs and native C DSP plugins. Move has a 128×64 1-bit display, 32 pads,
8 knobs, a jog wheel, and various buttons.

---

## 🔗 Quick Reference: Device Access

**Access Ableton Move Device Screen:**
```
http://move.local:7681
```
Open in Chrome (with Claude extension for automated screenshots) to see live 128×64 OLED display.

---

Before writing any code, always understand:
1. What **type** of module is this? (sound generator, audio FX, MIDI FX, utility, tool, overtake)
2. Does it need **audio/DSP** (requires native C plugin), or is it **UI-only** (JavaScript only)?
3. Should it be **chainable** in Signal Chain?

---

## Module Types

Choose `component_type` in `module.json`:

| Type | Description | Menu Location |
|------|-------------|---------------|
| `sound_generator` | Produces audio (synth, sampler) | Main menu, by category |
| `audio_fx` | Processes audio (reverb, delay) | Main menu, by category |
| `midi_fx` | Transforms MIDI (chord, arp) | Main menu, by category |
| `utility` | General purpose utilities | Main menu |
| `tool` | Interactive tools (file browser, sequencer) | Tools menu |
| `overtake` | Full control of Move's hardware UI | Overtake menu (shadow mode) |
| `system` | System modules (Module Store) — avoid for external modules | Shown last |

---

## Required Files

### module.json (always required)

```json
{
    "id": "my-module",
    "name": "My Module",
    "version": "0.1.0",
    "abbrev": "MOD",
    "description": "What the module does",
    "author": "Your Name",
    "ui": "ui.js",
    "api_version": 2,
    "component_type": "sound_generator",
    "capabilities": {
        "audio_out": true,
        "midi_in": true,
        "chainable": true
    }
}
```

**Critical rules for module.json:**
- Use `api_version: 2` for all new modules (required for Signal Chain)
- `id` must be lowercase hyphenated (`my-module`, not `MyModule`)
- The JSON parser is minimal: use double-quoted keys, lowercase `true`/`false`, no comments
- Keep it under 8KB
- `abbrev` is the 3–6 char display name shown in Signal Chain slot headers

**Capability flags:**

| Flag | When to use |
|------|-------------|
| `audio_out` | Module produces audio |
| `audio_in` | Module uses audio input |
| `midi_in` | Module processes MIDI |
| `midi_out` | Module sends MIDI |
| `chainable` | Module can be used in Signal Chain |
| `claims_master_knob` | Module handles CC 79 (volume knob) itself |
| `raw_midi` | Bypass host velocity/aftertouch transforms |
| `raw_ui` | Module owns Back button — must call `host_return_to_menu()` manually |
| `aftertouch` | Module uses aftertouch |
| `skip_led_clear` | Host skips LED clearing on load/unload |

---

## JavaScript UI (ui.js)

Every UI module exports four lifecycle functions via `globalThis`:

```javascript
import {
    MoveMainKnob, MoveShift, MoveMenu, MoveBack,
    MovePad1, MovePad32, MidiNoteOn, MidiCC, MidiNoteOff
} from '../../shared/constants.mjs';
import { shouldFilterMessage, decodeDelta, setLED, setButtonLED, clearAllLEDs } from '../../shared/input_filter.mjs';

// Module state
let selectedIndex = 0;

globalThis.init = function() {
    console.log("my-module: init");
    clearAllLEDs();
    clear_screen();
    redraw();
}

globalThis.tick = function() {
    // Called ~44x/sec. Keep it lightweight.
    // Good for animations, polling state changes.
}

globalThis.onMidiMessageInternal = function(data) {
    // Hardware input: pads, knobs, buttons, jog wheel
    if (shouldFilterMessage(data)) return;

    const status = data[0];
    const cc = data[1];
    const value = data[2];

    const isCC = (status & 0xF0) === 0xB0;
    const isNoteOn = (status & 0xF0) === 0x90 && value > 0;

    // Ignore capacitive touch from knobs (notes 0-9)
    if (!isCC && cc < 10) return;

    if (isCC && cc === MoveMenu) { /* menu pressed */ }
    if (isCC && cc === MoveBack) { host_return_to_menu(); }
}

globalThis.onMidiMessageExternal = function(data) {
    // External USB MIDI input (overtake modules only by default)
}

function redraw() {
    clear_screen();
    print(2, 2, "My Module", 1);
    // ... draw your UI
}
```

**Import paths from ui.js:** Use `../../shared/` (two levels up from `src/modules/your-module/`)

---

## Display API

The display is 128×64 pixels, 1-bit (black=0, white=1).

```javascript
// Direct functions
clear_screen()
print(x, y, text, color)       // color: 0=black, 1=white, 2=invert
set_pixel(x, y, value)
draw_rect(x, y, w, h, value)
fill_rect(x, y, w, h, value)
text_width(text)                // returns pixel width

// OOP style (also available)
display.clear()
display.drawText(x, y, text, color)
display.fillRect(x, y, w, h, value)
display.drawRect(x, y, w, h, value)
display.drawLine(x1, y1, x2, y2, value)
display.flush()                  // force immediate update
```

**Layout conventions** (standard menu layout via `menu_layout.mjs`):
- Header: y=0–11 (12px)
- List area: y=12–52 (or y=12–63 without footer)
- Footer: y=53–63 (11px)

For any settings/list UI, **use the shared menu system** — don't reinvent it:

```javascript
import { drawMenuHeader, drawMenuList, drawMenuFooter, menuLayoutDefaults } from '../../shared/menu_layout.mjs';
import { createMenuState, handleMenuInput } from '../../shared/menu_nav.mjs';
import { createMenuStack } from '../../shared/menu_stack.mjs';
import { createEnum, createValue, createToggle, createAction, createBack, createSubmenu } from '../../shared/menu_items.mjs';
```

---

## MIDI Hardware Mapping

```
Pads:       Notes 68–99 (bottom-left=68, top-right=99, 4 rows × 8 cols)
Steps:      Notes 16–31
Tracks:     CC 40–43 (reversed: CC43=Track1, CC40=Track4)

Key CCs:
  3   = Jog wheel click (127=pressed)
  14  = Jog wheel rotate (1-63=CW, 65-127=CCW)
  49  = Shift (127=held)
  50  = Menu
  51  = Back
  52  = Capture
  54  = Down arrow
  55  = Up arrow
  62  = Left arrow
  63  = Right arrow
  71–78 = Knobs 1–8 (relative: 1-63=CW, 65-127=CCW)
  79  = Master volume knob (relative encoder)
  85  = Play
  86  = Record

Notes 0–9 = Capacitive touch from knobs → always filter unless you need them
```

**Decoding encoder/jog wheel:**
```javascript
import { decodeDelta, decodeAcceleratedDelta } from '../../shared/input_filter.mjs';

const delta = decodeDelta(value);               // Simple ±1 for navigation
const accel = decodeAcceleratedDelta(value, 'jog');  // ±1 to ±10, for value editing
```

---

## LED Control

```javascript
import { setLED, setButtonLED, clearAllLEDs } from '../../shared/input_filter.mjs';
import { Black, White, Red, Blue, BrightGreen, BrightRed, LightGrey } from '../../shared/constants.mjs';

setLED(note, color);        // Pad/step LEDs (note-based)
setButtonLED(cc, color);    // Button LEDs (CC-based)
clearAllLEDs();
```

LED values are cached — redundant sends are suppressed automatically.

Common color constants (from `constants.mjs`):
- `Black = 0`, `White = 120`, `LightGrey = 118`
- `Red = 127`, `BrightRed = 1`, `Blue = 125`, `BrightGreen = 8`

---

## Overtake Modules

Overtake modules take **full control** of Move's UI. They run in shadow mode only.

Key requirements:
- Set `"component_type": "overtake"` in module.json
- Use **progressive LED initialization** — the MIDI buffer holds only ~64 packets per frame:

```javascript
const LEDS_PER_FRAME = 8;
let ledInitPending = true;
let ledInitIndex = 0;

const ALL_LEDS = [/* your pad LED list */];

function setupLedBatch() {
    const end = Math.min(ledInitIndex + LEDS_PER_FRAME, ALL_LEDS.length);
    for (let i = ledInitIndex; i < end; i++) {
        setLED(ALL_LEDS[i].note, ALL_LEDS[i].color);
    }
    ledInitIndex = end;
    if (ledInitIndex >= ALL_LEDS.length) ledInitPending = false;
}

globalThis.tick = function() {
    if (ledInitPending) setupLedBatch();
    drawUI();
};
```

- The host-level escape is **Shift + Volume Touch + Jog Click** — never block this
- If communicating with external USB devices, send your init handshake in `init()` proactively; the device may have already sent its greeting during the ~500ms load delay

---

## Tool Modules

Tool modules appear in the Tools menu. Add `tool_config` to module.json:

```json
{
    "component_type": "tool",
    "tool_config": {
        "interactive": true,
        "skip_file_browser": true
    }
}
```

- `interactive: true` — module takes over the UI like an overtake
- Call `host_exit_module()` (not `host_return_to_menu()`) to exit back to the tools menu
- For file-accepting tools: set `input_extensions: [".wav"]` and omit `skip_file_browser`

---

## Native DSP Plugin (C)

For audio synthesis or processing, create a C plugin. Always use **API v2**:

```c
#include "host/plugin_api_v1.h"  // v2 is defined here too

typedef struct {
    float sample_rate;
    // your synth state
} my_plugin_t;

static void* create_instance(const char *module_dir, const char *json_defaults) {
    my_plugin_t *inst = calloc(1, sizeof(my_plugin_t));
    inst->sample_rate = 44100.0f;
    return inst;
}

static void destroy_instance(void *instance) { free(instance); }

static void on_midi(void *instance, const uint8_t *msg, int len, int source) {
    // source: 0=internal (Move), 1=external (USB-A)
}

static void set_param(void *instance, const char *key, const char *val) { }

static int get_param(void *instance, const char *key, char *buf, int buf_len) {
    // Return chain_params and ui_hierarchy here for Signal Chain support
    return -1;
}

static void render_block(void *instance, int16_t *out_lr, int frames) {
    // 44100 Hz, 128 frames, stereo interleaved int16: [L0,R0,L1,R1,...]
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

plugin_api_v2_t* move_plugin_init_v2(const host_api_v1_t *host) {
    return &api;
}
```

**Build command** (add to `scripts/build.sh`):
```bash
"${CROSS_PREFIX}gcc" -g -O3 -shared -fPIC \
    src/modules/your-module/dsp/plugin.c \
    -o build/modules/your-module/dsp.so \
    -Isrc -lm
```

Audio spec: 44100 Hz, 128 frames/block, stereo interleaved int16.

---

## JS ↔ DSP Communication

```javascript
// In ui.js
host_module_set_param("cutoff", "64");
const val = host_module_get_param("cutoff");
```

```c
// In plugin .c
static void set_param(void *inst, const char *key, const char *val) {
    if (strcmp(key, "cutoff") == 0) inst->cutoff = atoi(val);
}
static int get_param(void *inst, const char *key, char *buf, int buf_len) {
    if (strcmp(key, "cutoff") == 0) {
        return snprintf(buf, buf_len, "%d", inst->cutoff);
    }
    return -1;
}
```

---

## Signal Chain Integration

For a module to work inside Signal Chain patches, it needs both `"chainable": true` in capabilities and the `get_param` handler to expose `chain_params` and `ui_hierarchy`.

### chain_params (parameter metadata for Shadow UI knobs)

```c
if (strcmp(key, "chain_params") == 0) {
    const char *json = "["
        "{\"key\":\"cutoff\",\"name\":\"Cutoff\",\"type\":\"int\",\"min\":0,\"max\":127,\"default\":64},"
        "{\"key\":\"mode\",\"name\":\"Mode\",\"type\":\"enum\",\"options\":[\"LP\",\"HP\",\"BP\"]}"
    "]";
    strncpy(buf, json, buf_len - 1);
    return strlen(json);
}
```

### ui_hierarchy (menu structure for Shadow UI navigation)

```c
if (strcmp(key, "ui_hierarchy") == 0) {
    const char *json = "{"
        "\"levels\":{"
            "\"root\":{"
                "\"name\":\"MySynth\","
                "\"knobs\":[\"cutoff\",\"mode\"],"
                "\"params\":["
                    "{\"key\":\"cutoff\",\"name\":\"Cutoff\",\"type\":\"int\",\"min\":0,\"max\":127},"
                    "{\"key\":\"mode\",\"name\":\"Mode\",\"type\":\"enum\",\"options\":[\"LP\",\"HP\",\"BP\"]}"
                "]"
            "}"
        "}"
    "}";
    strncpy(buf, json, buf_len - 1);
    return strlen(json);
}
```

---

## File System & Device Constraints

**⚠️ CRITICAL: Never write to `/tmp` on device.** The root filesystem is ~463MB and nearly always 100% full. `/tmp` is on rootfs.

Always use `/data/UserData/` paths:
```javascript
// Good
host_write_file("/data/UserData/schwung/my-module/state.json", JSON.stringify(state));

// BAD — will fill the disk
host_write_file("/tmp/state.json", data);
```

**Useful file system host functions:**
```javascript
host_file_exists(path)          // bool
host_read_file(path)            // string or null
host_write_file(path, content)  // bool
host_ensure_dir(path)           // mkdir -p, bool
host_remove_dir(path)           // rm -rf, bool
```

---

## Deployment

**Never scp individual files.** Always use the install script:
```bash
./scripts/install.sh local --skip-modules --skip-confirmation
```

The install script handles setuid, symlinks, feature config, and service restart.

After deploying, enable logging and check on device:
```bash
ssh ableton@move.local "touch /data/UserData/schwung/debug_log_on"
ssh ableton@move.local "tail -f /data/UserData/schwung/debug.log"
```

---

## Shared Utilities Reference

All located in `src/shared/` — import with `../../shared/` from a module's ui.js:

| File | Purpose |
|------|---------|
| `constants.mjs` | MIDI CC/note numbers, LED color values |
| `input_filter.mjs` | `shouldFilterMessage`, delta decoding, LED helpers |
| `menu_layout.mjs` | `drawMenuHeader`, `drawMenuList`, `drawMenuFooter` |
| `menu_nav.mjs` | `createMenuState`, `handleMenuInput` |
| `menu_stack.mjs` | `createMenuStack` for hierarchical menus |
| `menu_items.mjs` | `createEnum`, `createValue`, `createToggle`, `createAction`, `createBack`, `createSubmenu` |
| `move_display.mjs` | Additional display utilities |
| `logger.mjs` | Unified logging for both JS and C |
| `filepath_browser.mjs` | File/folder browser component |
| `text_entry.mjs` | On-screen keyboard for text input |
| `sampler_overlay.mjs` | Quantized sampler UI |
| `screen_reader.mjs` | Accessibility: `announce`, `announceMenuItem`, `announceView` |

---

## Deployment & Troubleshooting

### Connect to Move Device

#### SSH Access
```bash
# SSH into device
ssh ableton@move.local

# Check device filesystem
ssh ableton@move.local 'ls -la /data/UserData/schwung/'

# Check if module exists
ssh ableton@move.local 'find /data/UserData -name "your-module*" 2>/dev/null'
```

#### Web Display Access & Screenshotting via Chrome

Access the Move device's live display via HTTP and use Chrome browser tools to capture it:

```
http://move.local:7681
```

**Method 1: Using Chrome Browser Tools (Automated - Recommended for LLMs)**

1. **Ensure Chrome is open with Claude extension** and has network access to the device
2. **Navigate to the display:** Claude will automatically connect and navigate to `http://move.local:7681`
3. **Capture the display:** Claude uses the zoom tool to capture only the grey OLED display box in the center of the webpage (128×64 pixel area)
4. **Screenshot shows only the device display** - clean, focused, easy to analyze

**Example of what Claude sees:**
- Full browser screenshot showing the device display interface
- Zoom into just the grey-outlined box showing the actual OLED content
- Displays current module UI (parameters, values, formatting)

**Method 2: Manual Screenshot (Mac/Windows)**

1. **Open in Safari/Chrome:** `http://move.local:7681`
2. **Screenshot just the display area:**
   - **Mac:** Cmd+Shift+4 → select the grey box → saved to Desktop
   - **Windows:** Windows+Shift+S → select the grey box → save or copy to clipboard
3. **Save screenshot to project folder:**
   ```bash
   mkdir -p ~/Desktop/Move/BD-crusher/screenshots
   mv ~/Desktop/Screenshot*.png ~/Desktop/Move/BD-crusher/screenshots/
   ```

**How Claude Uses Screenshots for Troubleshooting:**

When you share a screenshot, Claude can:
- ✅ See the actual device display output pixel-by-pixel
- ✅ Compare against expected UI formatting (e.g., "Resample should be `26040 Hz` not `26040.00`")
- ✅ Identify rendering issues: truncation, decimal formatting, missing suffixes (dB, Hz, kHz)
- ✅ Verify if code changes have been applied to the device
- ✅ Provide targeted fixes based on what's visually wrong vs. expected

**Example diagnosis:**
- Screenshot shows: "Resample: 26040.00"
- Expected: "Resample: 26040"
- Claude identifies: ui.js formatting function not applied on device → recommends rebuild/redeploy

### Build & Deploy Module

#### Option 1: Build Locally (Cross-Compiler)

Requires ARM64 cross-compiler toolchain installed:

```bash
# Navigate to module directory
cd src/your-module

# Build with cross-compiler prefix for ARM64
CROSS_PREFIX=aarch64-unknown-linux-gnu- ./scripts/build.sh

# Output: dist/your-module-module.tar.gz (ready to deploy)
```

**Prerequisites:**
```bash
# Install cross-compiler on macOS (via Homebrew)
brew install aarch64-unknown-linux-gnu-gcc

# Or on Linux
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

#### Option 2: Build with Docker

No toolchain installation needed — Docker handles it:

```bash
# Build inside Docker container (ARM64 target)
docker run --rm -v $(pwd):/workspace -w /workspace/src/your-module \
  ubuntu:22.04 bash -c \
  'apt update && apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu && \
   CROSS_PREFIX=aarch64-unknown-linux-gnu- ./scripts/build.sh'

# Output: dist/your-module-module.tar.gz (ready to deploy)
```

#### Deploy to Device

After building (using either method):

```bash
# Deploy to device at /data/UserData/schwung/
scp dist/your-module-module.tar.gz ableton@move.local:/data/UserData/schwung/

# Extract on device
ssh ableton@move.local 'cd /data/UserData/schwung && \
  tar -xzf your-module-module.tar.gz && \
  rm your-module-module.tar.gz'

# Verify deployment
ssh ableton@move.local 'ls -la /data/UserData/schwung/modules/audio_fx/your-module/'
```

#### Complete Build → Deploy Workflow

```bash
# 1. Build locally
cd src/your-module
CROSS_PREFIX=aarch64-unknown-linux-gnu- ./scripts/build.sh

# 2. Deploy to device
scp dist/your-module-module.tar.gz ableton@move.local:/data/UserData/schwung/
ssh ableton@move.local 'cd /data/UserData/schwung && tar -xzf your-module-module.tar.gz && rm your-module-module.tar.gz'

# 3. Clear cache and reload
ssh ableton@move.local 'rm -rf /data/UserData/schwung/modules/.cache /data/UserData/schwung/modules/__pycache__'

# 4. Reboot device
ssh root@move.local 'reboot'

# 5. View device display to verify
# Open: http://move.local:7681 in Chrome
```

### Clear Cache & Reload

```bash
# Clear browser/UI cache on device
ssh ableton@move.local 'rm -rf /data/UserData/schwung/modules/.cache /data/UserData/schwung/modules/__pycache__'

# Verify changes are on device (e.g., for ui.js modifications)
ssh ableton@move.local 'grep -n "your_search_string" /data/UserData/schwung/modules/audio_fx/your-module/ui.js'

# Check actual module location
ssh ableton@move.local 'find /data/UserData/schwung -name "ui.js" -o -name "module.json"'

# Restart the device (verified working - requires root access)
ssh root@move.local 'reboot'
```

### Debug & Verify Code

```bash
# Verify ui.js is on device with changes
ssh ableton@move.local 'cat /data/UserData/schwung/modules/audio_fx/your-module/ui.js | head -50'

# Verify tarball contains correct files before deploy
tar -tzf dist/your-module-module.tar.gz | head -20

# Check DSP plugin was built
ssh ableton@move.local 'file /data/UserData/schwung/modules/audio_fx/your-module/your-module.so'
# Expected: ELF 64-bit LSB shared object, ARM aarch64

# Enable logging on device
ssh ableton@move.local 'touch /data/UserData/schwung/debug_log_on'

# View logs
ssh ableton@move.local 'tail -f /data/UserData/schwung/debug.log'
```

### Common Issues & Solutions

| Issue | Check | Solution |
|-------|-------|----------|
| UI changes not appearing after deploy | `grep -n "search_term" /data/UserData/schwung/modules/.../ui.js` | Verify file is on device with your changes. If not, redeploy to correct path: `/data/UserData/schwung/` not `/data/UserData/` |
| Module shows in menu but won't load | `file .so` shows wrong arch | Rebuild with correct `CROSS_PREFIX=aarch64-unknown-linux-gnu-` |
| Display/UI not updating | Check cache exists | Run: `ssh ableton@move.local 'rm -rf /data/UserData/schwung/modules/.cache'` then reload module |
| "Module not found" in menu | Check module.json valid JSON | Ensure: double-quoted keys, lowercase `true`/`false`, no comments, under 8KB |
| DSP params not responding | `get_param` not implemented | Ensure `get_param()` returns correct JSON for `chain_params` and `ui_hierarchy` |

### Reload Module on Device

After deploying changes:
1. **Close the module** on Move (press Back)
2. **Reopen it** from the menu
3. OR **Restart Ableton** on the device
4. OR **Hard refresh** browser (Cmd+Shift+R on Mac, Ctrl+Shift+R on Windows/Linux)

---

## Checklist: New Module

Before declaring a module ready, verify:

- [ ] `module.json`: valid JSON (double-quoted keys, lowercase booleans, no comments, under 8KB)
- [ ] `api_version: 2` set
- [ ] `component_type` set correctly
- [ ] `abbrev` set (3–6 chars) if this will appear in Signal Chain
- [ ] `ui.js` exports `init`, `tick`, `onMidiMessageInternal`, `onMidiMessageExternal` via `globalThis`
- [ ] Capacitive touch notes (0–9) filtered in `onMidiMessageInternal`
- [ ] `shouldFilterMessage` called before processing MIDI
- [ ] Back button handled (either via default host behavior, or `host_return_to_menu()` if `raw_ui`)
- [ ] Display cleared in `init()`
- [ ] No writes to `/tmp` — use `/data/UserData/` for any file I/O
- [ ] DSP plugin uses API v2 if DSP is included
- [ ] Overtake modules: use progressive LED initialization (max 8 LEDs/frame)
- [ ] Tool modules: use `host_exit_module()` to return to tools menu

---

## Common Patterns

### Simple pad grid

```javascript
globalThis.onMidiMessageInternal = function(data) {
    if (shouldFilterMessage(data)) return;
    const isNoteOn = (data[0] & 0xF0) === 0x90 && data[2] > 0;
    if (isNoteOn && data[1] >= 68 && data[1] <= 99) {
        const padIndex = data[1] - 68;  // 0-31
        handlePad(padIndex);
    }
}
```

### Jog wheel navigation

```javascript
import { decodeDelta } from '../../shared/input_filter.mjs';

if (cc === 14) {  // Jog wheel
    const delta = decodeDelta(value);  // -1 or +1
    selectedIndex = Math.max(0, Math.min(items.length - 1, selectedIndex + delta));
    redraw();
}
```

### Saving/loading state

```javascript
const STATE_PATH = "/data/UserData/schwung/my-module/state.json";

function saveState() {
    host_ensure_dir("/data/UserData/schwung/my-module");
    host_write_file(STATE_PATH, JSON.stringify({ preset, volume }));
}

function loadState() {
    const raw = host_read_file(STATE_PATH);
    if (raw) {
        try {
            const s = JSON.parse(raw);
            preset = s.preset ?? 0;
            volume = s.volume ?? 100;
        } catch(e) { console.log("my-module: failed to parse state"); }
    }
}
```