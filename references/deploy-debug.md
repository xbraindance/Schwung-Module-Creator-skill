# Deploy, Build & Debug

**When to load:** building the module, deploying to Move, watching logs, using the display mirror, enabling/disabling verbose logging, or debugging install-path mismatches.

## Authoritative upstream
- `docs/LOGGING.md` — unified logger, enable/disable, format
  — https://github.com/charlesvestal/schwung/blob/main/docs/LOGGING.md
- `BUILDING.md` — build system, cross-compilation
- `scripts/build.sh`, `scripts/install.sh` at the repo root

Optional private notes (may not exist on your machine):
`schwung-wiki/framework/deploy-patterns.md`,
`schwung-wiki/gotchas/deployment-path-gotchas.md`,
`schwung-wiki/gotchas/build-issues.md`,
`BD-1200/implementation/signal-chain-loading-gotchas.md`.

## Build

### Docker (reproducible, preferred)
```bash
docker build -t my-module-builder -f scripts/Dockerfile .
docker run --rm -v "$PWD:/build" -w /build my-module-builder ./scripts/build.sh
```

### Local macOS with Homebrew cross-toolchain
```bash
CROSS_PREFIX=aarch64-unknown-linux-gnu- ./scripts/build.sh
```

Output: `dist/<module>-module.tar.gz` containing `module.json`, `ui.js`, `dsp.so` (if native), `ui_chain.js` (if chainable), and any asset folders declared in `module.json`.

### Including asset folders in the tarball

Modules with user-uploadable assets need the build script to copy them:
```bash
if [ -d "src/custom_chords" ]; then
    mkdir -p "dist/$MODULE_ID/custom_chords"
    cp src/custom_chords/*.mid "dist/$MODULE_ID/custom_chords/" 2>/dev/null || true
fi
```

See `references/manifest.md` for the `assets` declaration.

## Deploy

```bash
scp dist/my-module-module.tar.gz ableton@move.local:/data/UserData/
ssh ableton@move.local 'tar -xzf /data/UserData/my-module-module.tar.gz \
  -C /data/UserData/schwung/modules/audio_fx/'
```

**Install path must match `component_type`** (see `references/manifest.md` table). Mismatch = module invisible.

| `component_type` | Extract into |
|---|---|
| `sound_generator` | `/data/UserData/schwung/modules/sound_generators/` |
| `audio_fx` | `.../modules/audio_fx/` |
| `midi_fx` | `.../modules/midi_fx/` |
| `utility` | `.../modules/utilities/` |
| `tool` | `.../modules/tools/` |
| `overtake` | `.../modules/overtake/` |

The packaged tarball is `<id>/...` (one top-level folder named for the module id, containing
`module.json`). Extracting into `.../audio_fx/` therefore lands it at `.../audio_fx/<id>/` —
correct, no `--strip-components` needed. Do **not** "rebuild flat": the Schwung Manager's
installer depends on that single wrapper folder (see next section).

## Schwung Manager install + Module Store catalog

Most users don't SSH — they install via the **Schwung Manager** web UI (`http://move.local:7700`
→ **Modules**). Three paths, all of which end up running the *same* packaged tarball:

- **Custom → from GitHub URL** — user pastes `owner/repo`. The Manager fetches
  `https://raw.githubusercontent.com/owner/repo/<main|master>/release.json`, then downloads its
  `download_url` (a release asset) and installs it.
- **Custom → from file** — user uploads a `.tar.gz`.
- **Catalog** — one-tap install for listed modules (see below).

**Installer tarball contract (this is the gotcha).** The Manager does, in effect:
`tar -xzf upload -C tmp; moduleDir = tmp/<first top-level entry>; read moduleDir/module.json`.
So the archive **must be a single top-level folder containing `module.json` at its root**
(`<id>/module.json` + `<id>/<id>.so` + …). It does **not** search recursively.

> **Error: "No module.json found in tarball".** The first top-level entry had no `module.json`
> in it. Causes, most common first:
> 1. User uploaded GitHub's auto-generated **"Source code (tar.gz)"** (`repo-<ver>/` with
>    `module.json` down in `src/`). Ship and point them at the packaged `<id>-module.tar.gz`
>    **release asset**, never the source archive.
> 2. Tarball is **flat** (files at the root, no `<id>/` wrapper) → fix the build to wrap.
> 3. Extra top-level entry sorts before `<id>/` — e.g. **macOS `tar` adds `._*` / `PaxHeader`
>    junk** from xattrs. Build the release tarball on Linux/CI, or `COPYFILE_DISABLE=1 tar …`.

**release.json** (repo root, on the default branch) is what the URL-installer and the catalog
read for version + asset — *not* the GitHub releases API:
```json
{ "version": "0.19.0",
  "download_url": "https://github.com/owner/repo/releases/download/v0.19.0/<id>-module.tar.gz" }
```
Bump `module.json` **and** `release.json` together, then push a `vX.Y.Z` tag so CI builds the
`.so`, packs `<id>-module.tar.gz`, and publishes the Release. Between merge and tag, `release.json`
points at a not-yet-published asset — tag promptly or installs 404.

**Getting into the catalog (Module Store).** The store reads
`https://raw.githubusercontent.com/charlesvestal/schwung/main/module-catalog.json`. Add an entry
via a **PR to `charlesvestal/schwung`** (third-party modules are welcome — many are external repos):
```json
{ "id": "your_id", "name": "Your Name", "description": "short", "author": "you",
  "component_type": "audio_fx", "github_repo": "owner/repo", "default_branch": "main",
  "asset_name": "your_id-module.tar.gz", "min_host_version": "0.3.0" }
```
`id` must match `module.json`'s `id` (used to match installed modules for updates). `0.3.0` is the
de-facto floor for `audio_fx`/`sound_generator` peers. The store then resolves the version live
from your repo's `release.json`, so the catalog entry never needs re-touching on each release.

## Reload

| Change | What to do |
|---|---|
| UI-only (`ui.js` / `ui_chain.js`) | `host_rescan_modules()` from shadow_ui, or reopen the picker |
| `module.json` | Rescan usually enough |
| DSP (`.so`) or any native code | **Full reboot** — `ssh root@move.local 'reboot'` |
| Cached wrong version showing up | `ssh ableton@move.local 'rm -rf /data/UserData/schwung/modules/<type>/<id>/'`, re-extract |

## Debug surfaces

- Display mirror — `http://move.local:7681` (live OLED view; jog the wheel once if it looks stale)
- Move Manager — `http://move.local:7700`
- Log file — `/data/UserData/schwung/debug.log`

## Enable / tail / clear logs

```bash
# Enable
ssh ableton@move.local 'touch /data/UserData/schwung/debug_log_on'

# Tail live
ssh ableton@move.local 'tail -f /data/UserData/schwung/debug.log'

# Disable (zero overhead when absent)
ssh ableton@move.local 'rm /data/UserData/schwung/debug_log_on'

# Truncate (log never auto-rotates — grows unbounded while enabled)
ssh ableton@move.local ': > /data/UserData/schwung/debug.log'
```

## Logging APIs

**JavaScript** (routes to `debug.log` automatically when enabled):
```js
console.log("my-module: init");
console.warn("...");
console.error("...");

// Or prefix automatically:
import { installConsoleOverride } from '../../shared/logger.mjs';
installConsoleOverride('my-module');   // now console.log prepends "[my-module]"
```

**C DSP**:
```c
#include "host/unified_log.h"
LOG_DEBUG("my-dsp", "cutoff=%f", inst->cutoff);
LOG_INFO("my-dsp", "preset loaded");
LOG_WARN("my-dsp", "...");
LOG_ERROR("my-dsp", "...");
```

Or use the `host->log(msg)` callback passed into `_init` / `create_instance`. Signature is single-arg `const char *` — for printf-style formatting use `snprintf` into a local buffer first, or use the `LOG_*` macros above. It is **not** realtime-safe — never call from `render_block` / `process_midi`. See `references/realtime.md`.

**Never** log from inside `render_block` / SPI callback path — that's the fastest way to cause audio dropouts.

## Verify deploy worked

```bash
# Check file landed
ssh ableton@move.local 'ls -la /data/UserData/schwung/modules/audio_fx/<id>/'

# Check .so exports the right symbol
ssh ableton@move.local \
  'nm -D /data/UserData/schwung/modules/audio_fx/<id>/dsp.so | grep -E "move_plugin_init_v2|move_audio_fx_init_v2|move_midi_fx_init"'

# Grep source for sanity
ssh ableton@move.local 'grep -n "some_unique_token" /data/UserData/schwung/modules/audio_fx/<id>/ui.js'
```

## Common fails

- Install path doesn't match `component_type` → module silently missing from picker
- `.so` not executable after tar extraction → `chmod +x dsp.so` or fix tarball perms on the build side
- Manager upload fails "No module.json found in tarball" → wrong archive (GitHub "Source code",
  or flat, or macOS `._*`/PaxHeader junk sorting first). Must be one `<id>/` folder w/ module.json
  at its root — see "Schwung Manager install" above
- Manual `tar -C .../audio_fx/` double-nested to `<id>/<id>/` → you extracted a wrapped tarball into
  `.../audio_fx/<id>/`; extract into `.../audio_fx/` instead (the wrapper IS the `<id>/` dir)
- UI edit not visible → try `host_rescan_modules()`, then full reboot; also check `mtime` on device matches local
- Changes to `.so` but no reboot → old DSP is still dlopen'd in the host
- Log file grew to gigabytes → truncate it
- Logging flag left on in production → disable when done (zero overhead when off)
