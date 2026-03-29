---
name: Schwung Module Creator Skill aka SMC
description: Build and troubleshoot Schwung modules for Ableton Move hardware with complete development workflow
codeInterpreter: false
---

# Schwung Module Creator Skill aka SMC

Build and troubleshoot **Schwung modules** for Ableton Move hardware with Claude AI. This skill provides a complete development loop: write code → deploy → test → debug → repeat, all within your Claude session.

**GitHub Source:** https://github.com/xbraindance/Schwung-Module-Creator-skill/blob/main/skill.md

> **Auto-Update:** This skill checks GitHub for updates each time it runs. If a newer version is available, it automatically updates itself before executing.

---

## Quick Navigation

**Just Starting Out?**
- Read: [Quick Start](#quick-start) (5 minutes)
- UI Patterns: Jump to [UI Development Patterns](#ui-development-patterns)
- Follow: Example workflow below

**Building UIs?**
- [UI Development Patterns](#ui-development-patterns) — 5+ patterns with code
- [Common UI Bugs & Fixes](#common-ui-bugs--fixes) — 6+ real issues solved
- Use `http://move.local:7681` to screenshot and debug visually
- [Debugging with Logs](#debugging-with-logs) — View device behavior in real-time

**Deploying Modules?**
- Jump to: [Build & Deployment](#build--deployment)
- Check: [Module Checklist](#module-checklist) at end

**Need API Reference?**
- Jump to: [JavaScript UI](#javascript-ui-uijs)
- DSP Plugin: [Native DSP Plugin](#native-dsp-plugin-c)
- Signal Chain: [Signal Chain Integration](#signal-chain-integration)

---

## What It Does

This skill makes you a **10x more productive Schwung developer** by automating common tasks:

### Code Development
- Create module structure (module.json, ui.js, DSP plugins)
- Review code for quality and correctness
- Reference complete Schwung API documentation (auto-updated from GitHub)
- Implement Signal Chain support with proper parameter metadata

### Device Interaction
- Build and deploy modules to your Move device via SSH
- Verify code changes on device in real-time
- Screenshot the device display (via Chrome Claude extension)
- Run SSH commands to debug, clear cache, restart device
- Manage files, check logs, enable debugging

### Troubleshooting & Debugging
- Diagnose why modules won't load
- View live device logs while developing
- Verify display rendering pixel-by-pixel
- Check file system and permissions
- Identify MIDI routing and LED issues

### UI Development Support
- Complete UI pattern gallery with working code examples
- Display measurements and safe zones
- Troubleshooting common UI bugs
- Animation and state management patterns
- Performance optimization tips

---

## Quick Start (5 minutes)

### Prerequisites

- Claude Code (web, desktop, or IDE extension)
- Ableton Move device on your network
- SSH access configured: `ssh ableton@move.local`
- Chrome extension (optional, for screenshot mirroring)

### Installation

#### Option A: Auto-Updating Skill (Recommended)

1. **Clone the skill repo:**
   ```bash
   cd ~/.claude/skills
   git clone https://github.com/xbraindance/Schwung-Module-Creator-skill schwung-module-creator
   ```

2. **Add hook to Claude Code settings** (`~/.claude/settings.json`):
   ```json
   {
     "hooks": {
       "skill:invoke": {
         "command": "bash ~/.claude/skills/schwung-module-creator/scripts/skill-update.sh",
         "silent": true
       }
     }
   }
   ```

3. **Test:**
   ```bash
   bash ~/.claude/skills/schwung-module-creator/scripts/skill-update.sh
   ```

#### Option B: Manual Installation

1. Download `skill.md` from the GitHub repo
2. In Claude Code: Customize → Skills → Upload → select the file
3. Start new session and say: "Use Schwung Module Creator skill"

### First Use

```
Use the Schwung Module Creator skill and help me create
a new synth module called "my-synth"
```

The skill will guide you through the entire process!

---

## Module Structure

```
src/modules/<id>/
  module.json       # Required: metadata and capabilities
  ui.js             # Required: JavaScript UI
  ui_chain.js       # Optional: Signal Chain UI
  dsp/plugin.c      # Optional: native DSP plugin
```

### module.json

```json
{
    "id": "my-module",
    "name": "My Module",
    "version": "0.1.0",
    "abbrev": "MOD",
    "description": "What it does",
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

**Component Types:**
- `sound_generator` — Synths, samplers
- `audio_fx` — Audio effects
- `midi_fx` — MIDI processors
- `utility` — General utilities
- `tool` — Interactive tools (file browser, sequencer)
- `overtake` — Full UI control in shadow mode

---

## JavaScript UI (ui.js)

### Lifecycle

```javascript
globalThis.init = function() {
    // Called once when module loads
    clear_screen();
    print(2, 2, "My Module", 1);
}

globalThis.tick = function() {
    // Called ~44x/sec (128 frames @ 44.1kHz)
}

globalThis.onMidiMessageInternal = function(data) {
    // Hardware input: pads, knobs, buttons, jog
    if (shouldFilterMessage(data)) return;
}

globalThis.onMidiMessageExternal = function(data) {
    // External USB MIDI (overtake only by default)
}
```

### Display API

```javascript
clear_screen()                          // Clear to black
print(x, y, text, color)               // color: 0=black, 1=white, 2=invert
set_pixel(x, y, value)
draw_rect(x, y, w, h, value)
fill_rect(x, y, w, h, value)
text_width(text)                       // Returns pixel width
display.flush()                        // Force immediate update
```

### MIDI Mapping

```javascript
// Pads: Notes 68-99 (32 pads)
// Steps: Notes 16-31 (16 steps)
// Tracks: CCs 40-43 (reversed)

// Key CCs:
3   = Jog click       14 = Jog turn      49 = Shift
50  = Menu            51 = Back          54 = Down        55 = Up
62  = Left            63 = Right         71-78 = Knobs 1-8
79  = Master volume   85 = Play          86 = Record      88 = Mute
```

### LED Control

```javascript
import { setLED, setButtonLED, clearAllLEDs } from '../../shared/input_filter.mjs';
import { Red, Blue, BrightGreen, White } from '../../shared/constants.mjs';

setLED(note, color);        // Pad LEDs
setButtonLED(cc, color);    // Button LEDs
```

### Host Functions

```javascript
// Module management
host_load_module(id)
host_unload_module()
host_return_to_menu()
host_module_set_param(key, val)
host_module_get_param(key)

// Display
host_flush_display()
host_set_refresh_rate(hz)

// File I/O (use /data/UserData/, NOT /tmp)
host_file_exists(path)
host_read_file(path)
host_write_file(path, content)
host_ensure_dir(path)
host_remove_dir(path)
```

### Logging

```javascript
// Simple
console.log("Something happened");
console.error("Error!");

// Or with imports
import { installConsoleOverride } from '../../shared/logger.mjs';
installConsoleOverride('my-module');
```

**Enable device logging:**
```bash
ssh ableton@move.local "touch /data/UserData/schwung/debug_log_on"
ssh ableton@move.local "tail -f /data/UserData/schwung/debug.log"
```

---

## UI Development Patterns

### Display Layout Reference

The 128×64 OLED display is divided into three zones:

```
┌────────────────────────────────────┐
│ 0                                128
│ ┌──────────────────────────────┐
│0├─ Header (y=0-11, 12px)       │
│ │  Title, status               │
│ │  Separator line at y=11      │
│ ├──────────────────────────────┤
│1├─ Content (y=12-52, 40px)     │
│2├─  Menu items, lists, values  │
│3├─  ~5 lines of text           │
│4├─  Interactive UI area        │
│5├──────────────────────────────┤
│6├─ Footer (y=53-63, 11px)      │
│ │  Status, help, breadcrumb    │
│ └──────────────────────────────┘
```

**Exact Measurements:**
```
Header:          y=0-11px    (12px tall)
  • Title x=2px, y=2px
  • Separator at y=11

Content:         y=12-52px   (40px tall)
  • Left margin: 2px
  • Right margin: 2px
  • Item height: 8px per line (including spacing)
  • Max width: ~120px for text

Footer:          y=53-63px   (11px tall)
  • Text y=54px (1px top padding)
  • Separator at y=52

Font:            ~4.8px per character (monospace)
  • Max chars/line: ~20-21 chars
  • Line spacing: 8px
```

### Pattern 1: Header + List + Footer (Most Common)

Used by Module Store, file browser, menus. From production code:

```javascript
globalThis.tick = function() {
    clear_screen();

    // Header
    print(2, 2, "Store", 1);
    fill_rect(0, 11, 128, 1, 1);  // Separator line

    // Content - list of items
    const items = ["Item 1", "Item 2", "Item 3"];
    for (let i = 0; i < items.length; i++) {
        const y = 15 + (i * 8);  // 8px spacing
        const isSelected = (i === selectedIndex);

        if (isSelected) {
            fill_rect(0, y - 1, 128, 8, 1);  // Highlight
            print(2, y, items[i], 0);  // Inverted text
        } else {
            print(2, y, items[i], 1);
        }
    }

    // Footer
    fill_rect(0, 52, 128, 1, 1);  // Separator
    print(2, 54, "Jog: select  Back: return", 1);
};
```

### Pattern 2: Parameter Editor with Knob Control

For synths and effects that need value adjustment:

```javascript
let paramValue = 64;  // 0-127

function drawParameterEditor() {
    clear_screen();

    // Header
    print(2, 2, "Edit Cutoff", 1);
    fill_rect(0, 11, 128, 1, 1);

    // Large value display (centered)
    const valueStr = paramValue.toString();
    const width = text_width(valueStr);
    print(64 - (width / 2), 24, valueStr, 1);

    // Visual bar showing value
    const barWidth = Math.floor((paramValue / 127) * 100);
    fill_rect(2, 35, barWidth, 3, 1);  // Filled
    draw_rect(2, 35, 100, 3, 1);        // Outline

    // Footer
    fill_rect(0, 52, 128, 1, 1);
    print(2, 54, "Knob1: adjust", 1);
}

globalThis.onMidiMessageInternal = function(data) {
    const cc = data[1];
    const value = data[2];

    // Knob 1 (CC 71)
    if (cc === 71) {
        const delta = value <= 63 ? 1 : -1;
        paramValue = Math.max(0, Math.min(127, paramValue + delta * 2));
        host_module_set_param("cutoff", paramValue.toString());
    }
};
```

### Pattern 3: Pad Grid with LED Feedback

For controllers and sequencers (from production controller/ui.js):

```javascript
let selectedPad = 0;
const PADS = Array.from({length: 32}, (_, i) => 68 + i);

function drawPadGrid() {
    clear_screen();
    print(2, 2, "Pad Control", 1);
    fill_rect(0, 11, 128, 1, 1);

    const padNote = PADS[selectedPad];
    const octave = Math.floor(padNote / 12) - 1;
    const names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
    const name = names[padNote % 12];

    print(2, 20, `Pad: ${name}${octave}`, 1);
    print(2, 32, `Note: ${padNote}`, 1);
}

function updateLEDs() {
    clearAllLEDs();

    // Set all pads with colors
    for (let i = 0; i < 32; i++) {
        const color = (i === selectedPad) ? White : LightGrey;
        setLED(PADS[i], color);
    }
}

globalThis.onMidiMessageInternal = function(data) {
    const isNote = (data[0] & 0xF0) === 0x90;
    const note = data[1];

    if (isNote && note >= 68 && note <= 99) {
        selectedPad = note - 68;
        updateLEDs();
    }
};
```

### Pattern 4: Long Labels with Auto-Scrolling

For menu items that don't fit (from production store/ui.js):

```javascript
import { getMenuLabelScroller } from '../../shared/text_scroll.mjs';

const scroller = getMenuLabelScroller();
let items = ["Very Long Module Name 1", "Another Long Title 2"];
let selectedIndex = 0;

function drawMenu() {
    clear_screen();
    print(2, 2, "Modules", 1);
    fill_rect(0, 11, 128, 1, 1);

    for (let i = 0; i < items.length; i++) {
        const y = 15 + (i * 8);
        const isSelected = (i === selectedIndex);

        // Get scrolled version of text (max 18 chars with auto-scroll)
        const displayText = scroller.getScrolledText(items[i], 18);

        if (isSelected) {
            fill_rect(0, y - 1, 128, 8, 1);
            print(2, y, displayText, 0);  // Inverted
        } else {
            print(2, y, displayText, 1);
        }
    }
}

globalThis.tick = function() {
    // Scroller handles timing and scroll position
    if (scroller.tick()) {
        drawMenu();  // Redraw if scroll position changed
    }
};

function onSelectionChanged() {
    scroller.setSelected(selectedIndex);  // Reset scroller
    drawMenu();
}
```

### Pattern 5: Text Input with On-Screen Keyboard

For text entry (from production text-test/ui.js):

```javascript
import {
    openTextEntry,
    isTextEntryActive,
    handleTextEntryMidi,
    drawTextEntry,
    tickTextEntry
} from '../../shared/text_entry.mjs';

let userText = "";

globalThis.tick = function() {
    if (isTextEntryActive()) {
        // Keyboard is open - delegate to keyboard handler
        tickTextEntry();
        drawTextEntry();
        return;
    }

    // Normal UI
    clear_screen();
    print(2, 2, "Text Entry", 1);
    print(2, 20, "Entered:", 1);
    print(2, 32, userText || "(empty)", 1);
    print(2, 54, "Jog: edit text", 1);
};

globalThis.onMidiMessageInternal = function(data) {
    const cc = data[1];
    const value = data[2];

    if (isTextEntryActive()) {
        handleTextEntryMidi(data);
        return;
    }

    // Jog click opens keyboard
    if (cc === 3 && value > 0) {
        openTextEntry({
            title: "Enter text",
            initialText: userText,
            onConfirm: (text) => {
                userText = text || "(empty)";
                host_module_set_param("text", userText);
            },
            onCancel: () => {
                console.log("Text entry cancelled");
            }
        });
    }
};
```

---

## Common UI Bugs & Fixes

### Bug 1: Text Truncates or Overflows

**Symptom:** Text appears cut off or runs into next line

**Root Cause:** Not checking text width before printing

**Solution:**
```javascript
const maxWidth = 100;
let label = "Very Long Label";

// Measure text width
if (text_width(label) > maxWidth) {
    // Truncate and add ellipsis
    while (text_width(label) > maxWidth - 12) {
        label = label.slice(0, -1);
    }
    label += "...";
}
print(2, 20, label, 1);
```

### Bug 2: Menu Items Overlap or Touch

**Symptom:** Text from one item appears to run into the next

**Root Cause:** Using 6px line height instead of 8px, or incorrect y calculation

**Solution:**
```javascript
// ❌ WRONG: 6px spacing causes overlap
const ITEM_HEIGHT = 6;
const y = 15 + (i * 6);

// CORRECT: 8px spacing (standard)
const ITEM_HEIGHT = 8;
const y = 15 + (i * 8);
```

### Bug 3: Display Looks Frozen or Stale

**Symptom:** Code changes don't appear on device, or display hasn't updated

**Root Cause:** Missing `clear_screen()` or display refresh timing issue

**Solution:**
```javascript
globalThis.tick = function() {
    clear_screen();  // Always start fresh
    // ... draw your UI
    // For immediate update (if needed):
    // display.flush();
};
```

Also try: **Jog the hardware wheel once** — this refreshes the display mirroring at `http://move.local:7681`.

### Bug 4: Selection Indicator Invisible

**Symptom:** Can't tell which item is selected in menu

**Root Cause:** Using same color for selected and unselected items

**Solution (inverted highlight):**
```javascript
for (let i = 0; i < items.length; i++) {
    const y = 15 + (i * 8);

    if (i === selectedIndex) {
        // Inverted highlight
        fill_rect(0, y - 1, 128, 8, 1);      // White background
        print(2, y, items[i], 0);             // Black text
    } else {
        print(2, y, items[i], 1);             // White text, black background
    }
}
```

### Bug 5: Knob Values Jump by Large Steps

**Symptom:** Turning knob slightly increments value by 10+ instead of 1-2

**Root Cause:** Incorrect delta decoding or missing step size

**Solution:**
```javascript
import { decodeDelta, decodeAcceleratedDelta } from '../../shared/input_filter.mjs';

if (cc === 71) {  // Knob 1
    // Simple: just ±1
    const delta = decodeDelta(value);
    paramValue += delta * 2;  // Control step size here

    // Or accelerated: ±1 to ±10 based on speed
    const accelDelta = decodeAcceleratedDelta(value, 'knob1');
    paramValue += accelDelta * 1;  // Acceleration already included
}
```

### Bug 6: Some LEDs Don't Light Up or Buffer Overflow

**Symptom:** LEDs flicker, don't turn on, or MIDI buffer overflow errors in logs

**Root Cause:** Sending too many LED commands at once (max 60-64 per frame)

**Solution (progressive LED initialization):**
```javascript
const LEDS_PER_FRAME = 8;  // Send 8 LEDs per frame
let ledInitPending = true;
let ledInitIndex = 0;

const ALL_LEDS = [
    { note: 68, color: White },
    { note: 69, color: LightGrey },
    // ... all your LEDs
];

globalThis.tick = function() {
    if (ledInitPending) {
        // Send only 8 LEDs this frame
        const end = Math.min(ledInitIndex + LEDS_PER_FRAME, ALL_LEDS.length);
        for (let i = ledInitIndex; i < end; i++) {
            setLED(ALL_LEDS[i].note, ALL_LEDS[i].color);
        }
        ledInitIndex = end;
        if (ledInitIndex >= ALL_LEDS.length) {
            ledInitPending = false;
        }
    }

    drawUI();
};
```

---

## Debugging with Logs

In addition to visual debugging via screenshots, you can see exactly what's happening on your device using the unified logger:

### Enable Logging on Device

```bash
ssh ableton@move.local "touch /data/UserData/schwung/debug_log_on"
```

### View Live Logs

```bash
ssh ableton@move.local "tail -f /data/UserData/schwung/debug.log"
```

The log will show messages like:
```
14:23:45.123 [DEBUG] [my-module] init called
14:23:45.456 [DEBUG] [my-module] MIDI note 68 received
14:23:45.789 [DEBUG] [my-module] Parameter set: cutoff=64
```

### Add Logging to Your Module

**In JavaScript:**
```javascript
globalThis.init = function() {
    console.log("my-module: init");  // Automatically goes to debug.log
}

globalThis.onMidiMessageInternal = function(data) {
    const cc = data[1];
    const value = data[2];
    console.log(`my-module: CC ${cc} = ${value}`);
}
```

**Or with the logger module:**
```javascript
import { installConsoleOverride } from '../../shared/logger.mjs';

// Call once at startup
installConsoleOverride('my-module');

// Now console.log automatically prefixes with [my-module]
console.log("Something happened");  // Logs as: [my-module] Something happened
```

**In C DSP:**
```c
#include "host/unified_log.h"

LOG_DEBUG("my-dsp", "Rendering block, cutoff=%f", inst->cutoff);
LOG_ERROR("my-dsp", "Buffer overflow detected");
```

### Disable Logging

When done debugging:
```bash
ssh ableton@move.local "rm /data/UserData/schwung/debug_log_on"
```

The log file can grow unbounded, so clear it periodically:
```bash
ssh ableton@move.local "> /data/UserData/schwung/debug.log"
```

---

## UI Development Tips

### Performance: Minimize Full Screen Redraws

**Don't do this (inefficient):**
```javascript
globalThis.tick = function() {
    clear_screen();  // Clears every frame
    // ... expensive calculations every frame
    for (let i = 0; i < 100; i++) {
        // CPU-heavy work in hot path
    }
};
```

**Do this (optimized):**
```javascript
let needsRedraw = true;

globalThis.tick = function() {
    if (!needsRedraw) return;  // Skip if nothing changed

    clear_screen();
    drawUI();

    needsRedraw = false;
};

globalThis.onMidiMessageInternal = function(data) {
    // ... update state
    needsRedraw = true;  // Mark for redraw on next tick
};
```

### Text Rendering Best Practices

**Pre-measure text, don't compute in loops:**
```javascript
// ❌ DON'T: Measuring every frame
globalThis.tick = function() {
    const w = text_width(label);  // Computed each tick
    print(2, 2, label, 1);
}

// DO: Measure once at init
let labelWidth = 0;

globalThis.init = function() {
    labelWidth = text_width(label);  // Compute once
}

globalThis.tick = function() {
    print(2, 2, label, 1);  // Use stored value
}
```

### State Management Pattern

Keep UI state organized and bounded:

```javascript
// Define parameter with all metadata
const PARAM_CUTOFF = {
    value: 64,
    min: 0,
    max: 127,
    step: 1,
    label: "Cutoff"
};

function adjustParam(delta) {
    PARAM_CUTOFF.value += delta * PARAM_CUTOFF.step;
    // Clamp to min/max
    PARAM_CUTOFF.value = Math.max(
        PARAM_CUTOFF.min,
        Math.min(PARAM_CUTOFF.max, PARAM_CUTOFF.value)
    );
}

function getParamDisplay() {
    const percent = Math.round((PARAM_CUTOFF.value / PARAM_CUTOFF.max) * 100);
    return `${PARAM_CUTOFF.label}: ${PARAM_CUTOFF.value} (${percent}%)`;
}
```

### Multi-State UI (Loading, Error, Content)

For modules that load data or have multiple states:

```javascript
const STATE_LOADING = 'loading';
const STATE_ERROR = 'error';
const STATE_READY = 'ready';

let currentState = STATE_LOADING;
let errorMessage = '';

globalThis.tick = function() {
    clear_screen();

    if (currentState === STATE_LOADING) {
        print(2, 2, "Loading...", 1);
        // Show loading animation
    } else if (currentState === STATE_ERROR) {
        print(2, 2, "Error", 1);
        print(2, 20, errorMessage, 1);
        print(2, 54, "Back: return", 1);
    } else {
        // Draw normal content
        drawContent();
    }
};
```

### LED Feedback Patterns

**Pad Selection (bright = selected, dim = unselected):**
```javascript
function updatePadLEDs() {
    clearAllLEDs();
    for (let i = 0; i < 32; i++) {
        const color = (i === selectedPad) ? White : LightGrey;
        setLED(68 + i, color);
    }
}
```

**Status Indicator (recording, active):**
```javascript
const STATUS_IDLE = 0;
const STATUS_RECORDING = 1;

function updateStatusLED() {
    if (status === STATUS_RECORDING) {
        setButtonLED(118, Red);  // Record button LED
    } else {
        setButtonLED(118, Black);
    }
}
```

### Animation Patterns

**Scrolling Text:**
```javascript
let scrollOffset = 0;
let scrollDirection = 1;

globalThis.tick = function() {
    scrollOffset += scrollDirection;

    // Bounce at edges
    if (scrollOffset <= 0 || scrollOffset >= 50) {
        scrollDirection *= -1;
    }

    clear_screen();
    print(2 + scrollOffset, 10, "Moving Text", 1);
};
```

**Blinking Indicator:**
```javascript
let blinkCounter = 0;
const BLINK_SPEED = 10;

globalThis.tick = function() {
    blinkCounter = (blinkCounter + 1) % (BLINK_SPEED * 2);
    const isVisible = blinkCounter < BLINK_SPEED;

    clear_screen();
    if (isVisible) {
        print(2, 2, "●", 1);  // Visible dot
    }
};
```

**Value Lerp (smooth animation):**
```javascript
let displayValue = 0;
let targetValue = 100;
const LERP_SPEED = 0.05;

globalThis.tick = function() {
    displayValue += (targetValue - displayValue) * LERP_SPEED;
    print(2, 2, `Value: ${Math.round(displayValue)}`, 1);
};
```

### Safe Text Area Calculations

**Always use measured widths:**
```javascript
// Text that fits
const safeChars = 18;  // ~18 chars fits safely at y=20

// For centered text
function printCentered(text, y) {
    const width = text_width(text);
    const x = Math.max(2, 64 - (width / 2));  // Center, but min 2px from edge
    print(x, y, text, 1);
}
```

---

## Native DSP Plugin (C)

### Plugin API v2 (Required for Signal Chain)

```c
#include "host/plugin_api_v1.h"

static void* create_instance(const char *module_dir, const char *json_defaults) {
    my_plugin_t *inst = calloc(1, sizeof(my_plugin_t));
    return inst;
}

static void render_block(void *instance, int16_t *out_lr, int frames) {
    // 44100 Hz, 128 frames, stereo interleaved int16
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

### Build

Add to `scripts/build.sh`:
```bash
"${CROSS_PREFIX}gcc" -g -O3 -shared -fPIC \
    src/modules/my-module/dsp/plugin.c \
    -o build/modules/my-module/dsp.so \
    -Isrc -lm
```

### State Persistence (for patches)

```c
static void set_param(void *inst, const char *key, const char *val) {
    if (strcmp(key, "state") == 0) {
        // Parse and restore full state from JSON
    }
}

static int get_param(void *inst, const char *key, char *buf, int buf_len) {
    if (strcmp(key, "state") == 0) {
        // Serialize full state to JSON
    }
}
```

---

## Signal Chain Integration

For modules to work in Signal Chain:

1. `"chainable": true` in capabilities
2. `get_param` returns `chain_params` and `ui_hierarchy`

### chain_params (Parameter Metadata)

```c
if (strcmp(key, "chain_params") == 0) {
    const char *json = "["
        "{\"key\":\"cutoff\",\"name\":\"Cutoff\",\"type\":\"float\",\"min\":0,\"max\":1,\"step\":0.01},"
        "{\"key\":\"mode\",\"name\":\"Mode\",\"type\":\"enum\",\"options\":[\"LP\",\"HP\",\"BP\"]}"
    "]";
    strncpy(buf, json, buf_len - 1);
    return strlen(json);
}
```

### ui_hierarchy (Menu Structure)

```c
if (strcmp(key, "ui_hierarchy") == 0) {
    const char *json = "{"
        "\"levels\":{"
            "\"root\":{"
                "\"name\":\"MySynth\","
                "\"knobs\":[\"cutoff\",\"resonance\"],"
                "\"params\":[\"cutoff\",\"resonance\",{\"level\":\"advanced\"}]"
            "}"
        "}"
    "}";
    strncpy(buf, json, buf_len - 1);
    return strlen(json);
}
```

---

## Build & Deployment

### Build

```bash
./scripts/build.sh           # Auto-detects Docker
./scripts/package.sh         # Create tarball
./scripts/clean.sh           # Clean build/
```

### Deploy to Device

**Ask user before building/deploying!**

```bash
scp dist/my-module-module.tar.gz ableton@move.local:/data/UserData/schwung/
ssh ableton@move.local 'cd /data/UserData/schwung && tar -xzf my-module-module.tar.gz'
ssh root@move.local 'reboot'  # For DSP changes only
```

### Verify

```bash
ssh ableton@move.local 'grep -n "search_string" /data/UserData/schwung/modules/.../ui.js'
ssh ableton@move.local 'cat /data/UserData/schwung/debug.log'
```

---

## Module Checklist

Before calling a module ready:

- [ ] `module.json` valid JSON (no comments, double-quoted keys, lowercase booleans)
- [ ] `api_version: 2` set
- [ ] `component_type` correct
- [ ] `ui.js` exports `init`, `tick`, `onMidiMessageInternal`, `onMidiMessageExternal`
- [ ] Back button handled
- [ ] Display cleared in `init()`
- [ ] Notes 0-9 filtered (capacitive touch)
- [ ] No writes to `/tmp` (use `/data/UserData/`)
- [ ] DSP uses API v2 (if included)
- [ ] Overtake: progressive LED init (8 LEDs/frame max)
- [ ] Tool modules: use `host_exit_module()`

---

## References

**Official Schwung Repository:**
- https://github.com/charlesvestal/schwung — Source code, releases
- https://github.com/xbraindance/Schwung-Module-Creator-skill — This skill's repo
- `src/modules/` — Real working examples (controller, store, text-test, etc.)

**Documentation:**
- `docs/API.md` — Full JavaScript API reference
- `docs/ARCHITECTURE.md` — How Schwung works internally
- `docs/LOGGING.md` — Unified logging guide
- `docs/SKILL-AUTO-UPDATE.md` — Auto-update architecture
- `CLAUDE.md` — Code style, build, module structure, Signal Chain

**Device Access:**
- `http://move.local:7681` — Live OLED display mirroring (essential for UI debugging!)
- `http://move.local/development/` — SSH setup, logs, performance monitoring
- SSH: `ssh ableton@move.local` (after SSH key setup)

---

## Support

**UI troubleshooting?** → Read `docs/UI-GUIDE.md` (has 6+ bug fixes)
**Module not loading?** → Check `module.json` is valid JSON
**Display frozen?** → Jog wheel once to refresh mirroring
**DSP changes not working?** → Requires full reboot: `ssh root@move.local 'reboot'`

---

**Version:** Auto-updated from GitHub
**Last Updated:** (Synced hourly if auto-update enabled)
