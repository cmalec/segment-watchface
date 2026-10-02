# Segment

A seven-segment digital watchface for the Pebble Time 2, built on the current RePebble SDK.

![Platform](https://img.shields.io/badge/platform-emery-blue) ![SDK](https://img.shields.io/badge/pebble--sdk-4.x-blue) ![Watchface](https://img.shields.io/badge/type-watchface-green)

## Features

- Large seven-segment LCD-style time display (DS-Digital font)
- Date display in five formats (DD/MM/YY, MM/DD/YY, YY-MM-DD, WED-25, JAN-WED-22)
- Bluetooth connection indicator beside the battery readout, with optional disconnect vibration
- Custom button labels (up to 8 characters each)
- Battery level indicator (with option to hide)
- Optional seconds display
- Optional hourly vibration
- **Weather row**: current temperature with the day's high/low as `19°(28°/12°)`, plus an icon for the WMO condition (clear, partly cloudy, cloudy, fog, drizzle, rain, showers, sleet, snow, thunderstorm — clear and partly cloudy swap to a moon after dark), from Open-Meteo via the phone's location
- Power-saving mode (no seconds, blink, vibes, BT badge or battery readout between configurable hours)
- Health step count display + heart rate on Time 2 (the row shows last night's sleep until the day reaches 400 steps; a setting turns that off so it always shows steps)
- Web-based settings page

> **Origins:** Segment started as a modernization of [91 Dub 4.0](https://github.com/orviwan/91-Dub-v4.0) by Orviwan.

## Target hardware

Built exclusively for the **Pebble Time 2** (platform `emery`, 200×228 px, 64-color, touch display). The layout, typography, vector art, and build target are native Emery definitions, not scaled legacy form factors.

## Building

### Prerequisites

- Linux or macOS (Windows via WSL)
- [uv](https://docs.astral.sh/uv/getting-started/installation/) Python package manager
- System dependencies (Ubuntu):
  ```sh
  sudo apt install nodejs libsdl2-2.0-0 libglib2.0-0 libpixman-1-0 zlib1g libsndio7.0
  ```
  (`libsdl2` is only needed if you want to run the emulator GUI locally.)

### Setup

```sh
# Install the Pebble CLI
uv tool install pebble-tool

# Install the SDK (one-time)
pebble sdk install latest

# Clone and build
git clone https://github.com/cmalec/segment-watchface.git
cd segment-watchface
pebble build
```

Output: `build/segment-watchface.pbw`

### Agent skill (`.agent/skills/pebble-watchface`)

This repo vendors the official **[pebble-watchface agent skill](https://github.com/coredevices/pebble-watchface-agent-skill)** from Core Devices under `.agent/skills/pebble-watchface/` (SKILL.md + API references + project templates + helper scripts for icons/previews/validation). AI coding agents (Claude Code, Hermes, etc.) can load it to follow the same build → QEMU → screenshot-verify workflow used to develop this watchface, including the QEMU hygiene rules (start `pebble logs` before install,
`pebble kill && pebble wipe` on stale emulator state) and the visual verification checklist. Only the skill directory is vendored — no samples/tutorials from the upstream repo.

### Test on the emulator

```sh
pebble install --emulator emery
pebble emu-bt-connection --emulator emery --connected no   # optional: stop the phone's
                                                           # weather from overwriting injections
tools/verify_weather_glyphs.py                             # icon glyph ↔ rendered pixels
```

The only supported emulator target is `emery`.
`tools/verify_weather_glyphs.py` needs a build and an installed app: it parses the
glyph table out of `src/c/decorations.c`, decodes the glyphs baked into the built
font resource, drives one AppMessage per condition, screenshots each and compares
the ink pixel-for-pixel.

## Installing on your watch

There are two paths, depending on whether your phone is set up for developer connections.

### Path A — via the Pebble mobile app (recommended)

1. **One-time phone setup:**
    - Install the new Pebble app from [repebble.com/app](https://repebble.com/app)
    - In the app: **Devices → tap the ⋯ menu → Enable Dev Connect → sign in with GitHub**

2. **One-time computer setup:**
   ```sh
   pebble login   # opens GitHub sign-in
   ```

3. **Build and push to the watch:**
   ```sh
   pebble build
   pebble install --cloudpebble
   ```

   The `.pbw` is sent through the Pebble cloud to your paired phone, which installs it on the watch.

### Path B — sideload the `.pbw` directly

If you just want the file:

1. `pebble build` → grab `build/segment-watchface.pbw`
2. Transfer it to your phone (email it to yourself, sync via Syncthing, `adb push`, etc.)
3. Open the `.pbw` on the phone with the Pebble app — it will install to the connected watch

### Setting it as the active watchface

On the watch: press **Up/Down** from the watchface to open the launcher → **Watchfaces** → select **Segment**.

## Settings page

Configurable from the Pebble mobile app (tap the gear on the watchface card in the app). The settings page is hosted on GitHub Pages:

**https://cmalec.github.io/segment-watchface/server/index.14.html**

It configures: the colour theme (the 91 Dub presets in `server/themes.json`), health, the sleep readout, seconds, date format, button labels, blinking colon, invert, bluetooth vibe/icon, hourly vibe, branding, battery display, power-save schedule, °C/°F, and shows a live preview of the face. The clock's 12/24-hour format is not a face setting — it comes from the watch's own time setting, which the face follows (and shows no AM/PM marker for); the page says so rather than offering a toggle. Palette/theme editing is not exposed yet (the watch keeps whatever colours it has). The page source lives in `server/` in this repo; GitHub Pages serves it straight from the repo root.

## Project layout

```
├── package.json          # Project metadata, app keys, target platforms
├── wscript               # Build rules (includes -Wno-error for legacy code)
├── src/
│   ├── c/                # C source (main.c, window.c, timedigits.c, settings.c, ...)
│   └── pkjs/index.js     # PebbleKit JS (phone side; weather fetch + settings webview)
├── resources/
│   ├── fonts/            # DS-Digital + Lucida Console TTFs, weather-icons.ttf,
│   │                     #   vollazee-font/Vollazee-2vx18.ttf (bottom labels)
│   └── images/           # Branding, menu icon PNGs
├── tools/                # Emulator screenshot debug helpers (ASCII dumps, shot
│                         #   diffing) and verify_weather_glyphs.py (icon glyphs)
├── .agent/skills/        # Agent skills: the vendored pebble-watchface set plus
│                         #   weather-icons (this repo's weather-row font)
└── server/               # The settings web page (index.14.html; the version in the
                          #   name busts the phone browser's cache — bump it on edits)
                          #   and themes.json, the 91 Dub colour presets the page's
                          #   theme picker offers
```

## Troubleshooting builds

**`MESSAGE_KEY_xxx undeclared` errors after pulling changes** — the build cache doesn't notice when `messageKeys` change in `package.json`, so the generated key header is stale. Wipe and reconfigure:

```sh
pebble build distclean && pebble build configure && pebble build
```

**A font's glyphs don't change after editing its `characterRegex`** — same cache: the font resource is keyed on the file, not the definition, so the first build's glyph set sticks. Regenerate it with the same `distclean` sequence, and check what was baked by parsing the resource (`build/emery/resources/fonts/*.reso` is a pickled `ResourceObject`).

Warnings (`-Wsign-compare`, `-Wunused-variable`, `-Wformat-truncation` in health.c/decorations.c/bluetooth.c) are inherited from the legacy codebase and non-fatal — they're on the list to clean up properly.

## Emery layout

The watchface uses the actual Time 2 coordinate system directly:

- **Geometry**: `src/c/_globals.h` defines native 200×228 rectangles and points. There is no legacy-platform dispatch or runtime scale factor.
- **Typography**: `src/c/fonts.c` loads the Emery-specific DS-Digital sizes (99px clock, 26px seconds) and Lucidia 14 for everything else. Nothing is swapped at runtime.
- **Vector art**: `src/c/vector.c` stores native Emery points. GPaths are created without mutating or scaling shared arrays.
- **Clock**: the same 99px seven-segment face in both modes, spanning the panel's inner width; enabling seconds does not shrink it. The seconds pair takes its own row underneath, centred on the panel, in the space the clock vacates by moving up.
- **Health**: steps (or sleep) sit on the top strip in the panel's left column, with heart rate on the row below it — the two never share a line, so a long step count can't reach the heart-rate readout. Values come from `HealthMetricStepCount`/`HealthMetricSleepSeconds` and `HealthMetricHeartRateBPM`.
- **Top strip**: the right-hand cluster is laid out right-to-left from the panel's inner edge — optional bluetooth badge, battery percentage, battery icon — and the column of text to its left is bounded by whatever the cluster occupies (`battery_top_reserve()`), so nothing is placed from a number measured against the screen edge.
- **Date**: five shapes — numeric (`DD/MM/YY`, `MM/DD/YY`, `YY-MM-DD`) and name-bearing (`WED-25`, `SEP-WED-25`). `format_date()` builds them from `tm` fields with English abbreviations spelled out in the source: the Lucida character set is ASCII, so a locale whose abbreviations carry accents would render blanks, and the rest of the chrome is English.
- **Weather**: the phone-side JS (`src/pkjs/index.js`) fetches the current temperature, the day's high/low and the WMO condition code from [Open-Meteo](https://open-meteo.com) (free, no API key) using the phone's geolocation, with an IP-based fallback. The watch maps the code to one of ten conditions (`weather_cond_from_wmo()`) and renders `now°(high°/low°)` on the clock's bottom row, left of the seconds. The condition icon is a glyph of the [Weather Icons](https://erikflowers.github.io/weather-icons/) font (`resources/fonts/weather-icons.ttf`, SIL OFL 1.1): `package.json` bakes only the twelve codepoints the face draws (~1.6 KB of font resource), and the glyph is a text layer in the readout's ink colour, so it follows the colour sets and invert like the text beside it. Clear and partly cloudy have sun and moon forms; the face swaps them at 19:00/07:00 (there is no sunrise data on the watch). Two layout rules come from the font and are easy to break silently: the icon's frame must be at least as wide as the widest glyph's advance (a narrower text layer renders an ellipsis instead), and it must be tall enough for the glyphs' ink, which runs past the font's line box. °C/°F is a settings option, converted phone-side so unit flips are instant.
- **Button labels**: `LIGHT`/`PREV`/`NEXT` are user text, capped at `LABEL_MAX` (8) glyphs and filtered to the characters the Lucida font carries — an empty label leaves just the arrow.
- **Bottom strip**: below the bottom rule the face reads as three black sections split by the water-resistance box, each with its own label — `Chrono`, `V1` (inside the box) and `Graph` — drawn in the Vollazee face (`resources/fonts/vollazee-font/Vollazee-2vx18.ttf`, 16px, only those ten glyphs are baked). They carry the button-label ink (`c_d7`), so they follow the colour set and invert with the rest of the chrome.
- Everything else, including the colour sets the watch stores, blink, power saving, and hourly vibration, remains part of the Emery face.

## Modernization notes

Changes made during the modernization from the original codebase:

- `appinfo.json` → `package.json` (current SDK project format)
- New `wscript` matching the current SDK template
- C sources moved `src/` → `src/c/`, JS moved `src/js/` → `src/pkjs/index.js` (`enableMultiJS: true` is required for the `src/pkjs/` layout)
- `targetPlatforms` now includes `emery`
- Fresh app UUID (so this build doesn't collide with the original on the watch)
- Message keys resolve through the SDK-generated `MESSAGE_KEY_*` symbols; `settings_process_tuple` is an if/else chain because those symbols are extern variables, not compile-time constants
- `wscript` adds `-Wno-error -Wno-zero-length-bounds -Wno-cast-function-type` to tolerate legacy code under the modern compiler — these should be fixed properly over time (tracked in issues)
