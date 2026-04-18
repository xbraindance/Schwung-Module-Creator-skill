# Schwung Module Creator Skill

Build and troubleshoot **Schwung modules** for Ableton Move hardware with Claude AI. This skill provides a complete development loop: write code → deploy → test → debug → repeat, all within your Claude session.

**GitHub Repository:** https://github.com/xbraindance/Schwung-Module-Creator-skill

---

## What It Does

This skill makes you a ** more productive Schwung developer** by automating common tasks:

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
- 5 complete UI patterns with working code examples
- 6+ common UI bugs with step-by-step fixes
- Complete display layout reference with pixel measurements
- Animation and state management patterns
- Performance optimization tips (dirty-flag, efficient rendering)
- LED feedback patterns (selection, status indicators)

---

### Installation

1. Download `skill.md` from https://github.com/xbraindance/Schwung-Module-Creator-skill/blob/main/skill.md

2. In Claude Code:
   - Click "Customize" (left navigation)
   - Click "Skills"
   - Click "+" and select "Upload a skill"
   - Upload the `skill.md` file

3. Start a new session and say: "Use SMC" or "run Schwung Module Creator skill"

#### Option A: Auto-Updating Skill (Recommended)

This installs the skill AND enables automatic updates from GitHub.


1. **Add the auto-update hook to Claude Code settings** (one-time):

   Open/edit `~/.claude/settings.json` and add this under `hooks`:
   ```json
   {
     "hooks": {
       "skill:invoke": {
         "description": "Auto-update Schwung skill from GitHub",
         "command": "bash ~/.claude/skills/schwung-module-creator/scripts/skill-update.sh",
         "silent": true
       }
     }
   }
   ```

2. **Test it works:**
   ```bash
   bash ~/.claude/skills/schwung-module-creator/scripts/skill-update.sh
   echo "✅ Setup complete!"
   ```

### First Use

In Claude Code, start your Schwung development session:

```
Use the Schwung Module Creator skill and help me create a new synth module called "my-synth"
```

The skill will:
- Show you the complete Schwung API
- Guide you through module.json setup
- Help you write ui.js and DSP plugins
- Deploy to your Move device
- Debug issues in real-time

---

## Features in Detail

### 📚 Complete API Reference

The skill includes comprehensive offline documentation for:
- **JavaScript API** — Display, MIDI, LEDs, host functions
- **DSP Plugin API** — Native C audio processing (API v2)
- **Signal Chain** — Parameter metadata, menu structure, state persistence
- **Module Lifecycle** — init(), tick(), MIDI handlers
- **Shared Utilities** — Menu system, file browser, logger, text input
- **Move Hardware** — MIDI mapping, LED colors, hotkeys

**Integrated UI Guide:** 1,100+ lines of UI development patterns, bug fixes, and optimization tips
- No external files needed — everything in skill.md
- Production code examples from real modules
- Copy-paste ready patterns
- Real solutions for common bugs

**Auto-Updated:** If you enable the hook, the API reference stays in sync with GitHub's latest docs.

### 🚀 Device Deployment

**One-command deployment:**
```javascript
// In Claude: "Deploy my-synth module to Move device"
```

The skill handles:
- Cross-compilation for ARM64
- SSH file transfer
- Cache clearing
- Device restart (if needed)
- Log verification

### 📺 Live Screenshot & Debugging

With Chrome Claude extension:
```javascript
// In Claude: "Take a screenshot of the Move display"
```

Shows the live 128×64 OLED display, pixel-perfect. Great for:
- Verifying UI rendering
- Comparing expected vs actual layout
- Identifying text truncation or formatting issues
- Checking LED status

### ✅ SSH Device Access

Automated device commands:
- `ssh ableton@move.local 'tail -f /data/UserData/schwung/debug.log'` — View live logs
- `ssh ableton@move.local 'rm -rf /data/UserData/schwung/modules/.cache'` — Clear cache
- `ssh root@move.local 'reboot'` — Restart device
- Check file permissions, verify module installation, inspect module.json

### 🎨 Comprehensive UI Development Guide

The skill now includes an extensive UI development guide baked directly into skill.md:

**5 Complete UI Patterns (with full code):**
1. Header + List + Footer (menu system)
2. Parameter editor with knob control
3. Pad grid with LED feedback
4. Long labels with auto-scrolling
5. On-screen keyboard text input

**6+ Common UI Bugs & Solutions:**
- Text truncation → Use `text_width()` before printing
- Overlapping items → Use 8px line height, not 6px
- Frozen display → Always call `clear_screen()`
- Invisible selection → Use inverted highlight
- Knob jumps → Correct delta decoding
- LED overflow → Progressive init (8 LEDs/frame max)

**Debugging with Logs:**
- Enable logging on device with one command
- View live logs in real-time
- JavaScript logging: `console.log()` and logger module
- C DSP logging with `LOG_DEBUG()` macros

**UI Optimization Tips:**
- Performance: dirty-flag pattern for efficient redraws
- Text: pre-measure, don't compute in loops
- State: parameter objects with bounds checking
- Animations: scrolling, blinking, lerp patterns
- Display: exact pixel measurements and safe zones

**Quick Navigation:**
Jump to any section directly:
- Building UIs? → [UI Development Patterns](#ui-development-patterns)
- Have a UI bug? → [Common UI Bugs & Fixes](#common-ui-bugs--fixes)
- Need to optimize? → [UI Development Tips](#ui-development-tips)
- Debugging issues? → [Debugging with Logs](#debugging-with-logs)

### 🔄 Auto-Update Capability

Once you set up the hook:
- **Automatic:** Checks GitHub hourly
- **Smart:** Only updates if changed (uses SHA256 hash comparison)
- **Fast:** <1ms after first check (no HTTP overhead)
- **Safe:** Backs up previous version before updating
- **Silent:** Runs in background, no notifications
- **Graceful:** Continues if GitHub is unreachable

---

## Move SSH Setup

### One-Time SSH Configuration

The skill can run SSH commands on your Move device. Set this up once:

1. **Generate an SSH key** (if you don't have one):
   ```bash
   ssh-keygen -t ed25519 -f ~/.ssh/move_ed25519 -N ""
   ```

2. **Request a challenge code from Move:**
   ```bash
   curl -s -X POST http://move.local/api/v1/challenge -d '{}'
   ```

3. **Look at your Move screen** — a 6-digit code will appear

4. **Submit the code:**
   ```bash
   curl -s -X POST http://move.local/api/v1/challenge-response \
     -H "Content-Type: application/json" \
     -d '{"secret":"YOUR_CODE"}'
   ```

5. **Go to Move's web interface** and add your SSH key:
   - Open `http://move.local/development/ssh` in your browser
   - Paste your public key from:
     ```bash
     cat ~/.ssh/move_ed25519.pub
     ```

6. **Test SSH access:**
   ```bash
   ssh -i ~/.ssh/move_ed25519 ableton@move.local
   # Should connect without password
   ```

That's it! Now the skill can SSH into your device automatically.

---

## Requirements

| Requirement | Purpose | Optional? |
|-------------|---------|-----------|
| Claude Code | Run the skill | ❌ Required |
| Ableton Move device | Deploy & test modules | ❌ Required |
| SSH access | Remote device commands | ❌ Required (see setup above) |
| Chrome + Claude extension | Screenshot device display | ✅ Optional (for debugging) |
| curl | Download files from GitHub | ✅ Optional (auto-update only) |

---

## Example Workflow

Here's a typical development session:

```
Claude: "Use Schwung Module Creator skill"

You: "Create a new sound generator module called 'bd-synth' with DSP"

Claude: [Creates module structure, shows module.json and ui.js templates]

You: "Add a cutoff parameter with MIDI CC 74 control"

Claude: [Updates ui.js to decode CC 74 and send it to DSP]

You: "Build and deploy to Move"

Claude: [Compiles C DSP, creates tarball, SSHs to device, extracts]

You: "Take a screenshot of the device"

Claude: [Opens http://move.local:7681, captures OLED display]

You: "The text is truncated. Fix the formatting"

Claude: [Updates ui.js text width calculation, redeploys]

You: "Screenshot again"

Claude: [Shows updated display — text now fits perfectly]

You: "Great! Save this to docs/screenshots/"

Claude: [Saves screenshot locally for your documentation]
```

All without leaving Claude. 🚀

---

## What's New (Latest Update)

**Version: 2.0.0 - UI Development Guide Integration**

This update significantly expands skill.md to be a complete, self-contained reference:

✨ **New Content:**
- 5 complete UI patterns with working code (Header+List, Parameter Editor, Pad Grid, Scrolling, Text Input)
- 6+ common UI bugs with step-by-step fixes
- Complete logging guide (enable, view, add logs to code)
- UI optimization tips (performance, animations, state management)
- Display measurements and safe zone reference
- LED feedback patterns
- Real code examples from production modules

📈 **Skill Size:**
- Before: ~800 lines (API reference only)
- After: 1,123 lines (complete development guide)
- All in one file, no external references needed

🎯 **Impact:**
- Developers can find UI patterns without searching multiple docs
- Common bugs now have explicit solutions
- Copy-paste ready code patterns
- Production examples to learn from
- Performance tips for smooth UIs

---

## Troubleshooting

**Having UI issues?** Check the [Common UI Bugs & Fixes](#common-ui-bugs--fixes) section in the skill for 6+ solutions.

### "Skill not found" or "Skill won't load"

**Solution:** Make sure you:
1. Uploaded the `skill.md` file correctly
2. Started a new chat session (skills load at session start)
3. Said "Use Schwung Module Creator skill" in your first message

### SSH connection fails: "move.local not found"

**Solution:**
- Ensure Move is on the same WiFi network
- Check Move is powered on
- Try IP address instead: `ssh ableton@192.168.1.X`
- See [Move SSH Setup](#move-ssh-setup) section above

### "module.json: Invalid JSON" error on device

**Solution:**
- Check for comments in module.json (not allowed)
- Ensure all keys use double quotes: `"id"` not `'id'`
- Verify boolean values are lowercase: `true` not `True`
- Use JSON validator: `cat module.json | jq .`

### Display screenshot shows black screen

**Solution:**
- Make sure Chrome Claude extension is installed
- Jog the Move hardware once to refresh display mirroring
- Try again: the mirroring may be stale

### DSP changes not taking effect

**Solution:**
- Changes to `.so` files require full device restart
- Run: `ssh root@move.local 'reboot'`
- Old `.so` stays in memory until reboot (Linux dlopen behavior)

### Auto-update not working

**Solution:**
- Check hook is in `~/.claude/settings.json`:
  ```bash
  cat ~/.claude/settings.json | grep -A 3 "skill:invoke"
  ```
- Verify script is executable:
  ```bash
  ls -l ~/.claude/skills/schwung-module-creator/scripts/skill-update.sh
  # Should show: -rwxr-xr-x
  ```
- Test manually:
  ```bash
  bash ~/.claude/skills/schwung-module-creator/scripts/skill-update.sh
  ```

---

## Advanced: Custom Configuration

### Change Auto-Update Check Interval

In `scripts/skill-update.sh`, change this line:
```bash
CHECK_INTERVAL=3600  # 1 hour
# Change to:
CHECK_INTERVAL=1800  # 30 minutes
# or
CHECK_INTERVAL=86400 # 1 day
```

### Disable Auto-Update

Comment out the hook in `~/.claude/settings.json`:
```json
{
  "hooks": {
    // "skill:invoke": { ... }  // Disabled
  }
}
```

### Use a Different GitHub Branch

Edit `scripts/skill-update.sh` and change:
```bash
GITHUB_RAW_URL="https://raw.githubusercontent.com/xbraindance/Schwung-Module-Creator-skill/main/skill.md"
# to your fork/branch:
GITHUB_RAW_URL="https://raw.githubusercontent.com/YOUR_USER/YOUR_FORK/develop/skill.md"
```

---

## Skill Content Overview

The skill.md file now includes everything you need for Schwung development:

**Section Quick Links:**
- **Quick Start** — Installation and first use (5 minutes)
- **Module Structure** — module.json and folder layout
- **JavaScript UI** — Complete API reference with examples
- **UI Development Patterns** — 5 working patterns with code
- **Common UI Bugs & Fixes** — Real solutions for 6+ issues
- **Debugging with Logs** — Enable, view, and add logging
- **UI Development Tips** — Performance, animations, state management
- **Native DSP Plugin** — C plugin API v2 with examples
- **Signal Chain Integration** — Parameter metadata and hierarchy
- **Build & Deployment** — Build, deploy, and verify modules
- **Module Checklist** — Pre-release verification
- **References** — Links to Schwung docs and device access

---

## Support & Resources

| Resource | Link |
|----------|------|
| **Schwung Official Repo** | https://github.com/charlesvestal/schwung |
| **Skill GitHub Repo** | https://github.com/xbraindance/Schwung-Module-Creator-skill |
| **Skill File** | skill.md (all-in-one reference) |
| **Move Device Web** | `http://move.local/` |
| **Display Live Mirror** | `http://move.local:7681` |
| **SSH Access** | `ssh ableton@move.local` |

**Quick Answers:**
- UI text truncating? → See [Common UI Bugs & Fixes](#common-ui-bugs--fixes)
- Need a code pattern? → See [UI Development Patterns](#ui-development-patterns)
- Module not loading? → See [Troubleshooting](#troubleshooting)
- Want to optimize? → See [UI Development Tips](#ui-development-tips)
- Module examples? → Check `src/modules/` in Schwung repo

---

## License

This skill is provided as-is for Schwung module development. Schwung is maintained by [charlesvestal](https://github.com/charlesvestal).

---

## Version Info

- **Skill Version:** Auto-updates from GitHub
- **Schwung Compatibility:** v0.1.0+
- **Last Updated:** (Auto-synced from GitHub)

To check for updates manually:
```bash
bash ~/.claude/skills/schwung-module-creator/scripts/skill-update.sh
```
