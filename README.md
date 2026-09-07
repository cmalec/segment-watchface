# Segment

A seven-segment digital watchface for the Pebble Time 2 and the wider Pebble family, built on the current RePebble SDK.

![Platform](https://img.shields.io/badge/platform-emery-blue) ![SDK](https://img.shields.io/badge/pebble--sdk-4.x-blue) ![Watchface](https://img.shields.io/badge/type-watchface-green)

## Features

- Large seven-segment LCD-style time display (DS-Digital font)
- Date display
- Bluetooth connection indicator with optional disconnect vibration
- Battery level indicator (with option to hide)
- Optional seconds display
- Optional hourly vibration
- **Day high/low temperature readouts** (bottom corners, from Open-Meteo via the phone's location)
- Power-saving mode (no seconds, blink, vibes, BT badge or battery readout between configurable hours)
- Health step count display + heart rate on Time 2
- Color customization
- Web-based settings page

> **Origins:** Segment started as a modernization of [91 Dub 4.0](https://github.com/orviwan/91-Dub-v4.0) by Orviwan — the layout, fonts, and spirit of the original carried over. All credit for the original design belongs to Orviwan.

## Target hardware

Built for the **Pebble Time 2** (platform `emery`, 200×228 px, 64-color) and back-compatible with `aplite`, `basalt`, `chalk`, and `diorite`.

| Platform  | Device                   | Display                  |
|-----------|--------------------------|--------------------------|
| `emery`   | Pebble Time 2            | 200×228, 64-color, touch |
| `basalt`  | Pebble Time / Time Steel | 144×168, 64-color        |
| `chalk`   | Pebble Time Round        | 180×180 round, 64-color  |
| `aplite`  | Classic / Steel          | 144×168, B/W             |
| `diorite` | Pebble 2                 | 144×168, B/W             |

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
```

Other emulator targets: `aplite`, `basalt`, `chalk`, `diorite`.

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

**https://cmalec.github.io/segment-watchface/server/index.10.html**

It configures: health, seconds, blinking colon, invert, bluetooth vibe/icon, hourly vibe, branding, battery display, power-save schedule, °C/°F, two color sets (switchable by time or tap), preset themes, and a live preview. The page source lives in `server/` in this repo; GitHub Pages serves it straight from the repo root.

## Project layout

```
├── package.json          # Project metadata, app keys, target platforms
├── wscript               # Build rules (includes -Wno-error for legacy code)
├── src/
│   ├── c/                # C source (main.c, window.c, timedigits.c, settings.c, ...)
│   └── pkjs/index.js     # PebbleKit JS (phone side; weather fetch + settings webview)
├── resources/
│   ├── fonts/            # DS-Digital + Lucida Console TTFs
│   └── images/           # Branding, menu icon PNGs
├── tools/                # Emulator screenshot debug helpers (ASCII dumps, shot diffing)
└── server/               # The settings web page (index.10.html + theme presets)
```

## Troubleshooting builds

**`MESSAGE_KEY_xxx undeclared` errors after pulling changes** — the build cache doesn't notice when `messageKeys` change in `package.json`, so the generated key header is stale. Wipe and reconfigure:

```sh
pebble build distclean && pebble build configure && pebble build
```

Warnings (`-Wsign-compare`, `-Wunused-variable`, `-Wformat-truncation` in health.c/decorations.c/bluetooth.c) are inherited from the legacy codebase and non-fatal — they're on the list to clean up properly.

## Emery (Pebble Time 2) adaptation

The original layout was hardcoded for basalt's 144×168 display. Emery's 200×228 screen is almost exactly **basalt × 1.389**, so the adaptation is a faithful upscale rather than a redesign:

- **Geometry**: all layout macros in `src/c/_globals.h` gained an emery branch via the `SCREEN_ELSE()` macro (emery / rect / round), scaled 1.389 from the basalt values.
- **Fonts**: DS-Digital scales linearly (~0.507 px glyph width per px size), so emery loads 99/78/31 px and Lucidia 14 px versions of the same fonts. The larger fonts are gated `"targetPlatforms": ["emery"]` in
  `package.json` — they exceed the 256-byte glyph limit of the older platforms (emery allows 512).
- **Vector art**: bluetooth, arrows, and the WR box are `GPath`s, scaled at init by `1.389 << 10` per point.
- **Heart rate**: the Time 2 has an optical HRM, so when Health is enabled an emery-only heart + BPM readout shows next to the step counter (`HealthMetricHeartRateBPM`). Two digits are displayed (≤99 BPM); higher readings are hidden rather than clipped.
- **Weather**: the phone-side JS (`src/pkjs/index.js`) fetches the day's high/low from [Open-Meteo](https://open-meteo.com) (free, no API key)
  using the phone's geolocation (with an IP-based fallback), refreshed hourly. Bottom-left shows the high, bottom-right the low; °C/°F is a
  settings option, converted phone-side so unit flips are instant (no refetch). The center box is decorative.
- Everything else (color sets, blink, powersave, hourly vibe, etc.) works as in the original.

## Modernization notes

Changes made during the modernization from the original codebase:

- `appinfo.json` → `package.json` (current SDK project format)
- New `wscript` matching the current SDK template
- C sources moved `src/` → `src/c/`, JS moved `src/js/` → `src/pkjs/index.js` (`enableMultiJS: true` is required for the `src/pkjs/` layout)
- `targetPlatforms` now includes `emery`
- Fresh app UUID (so this build doesn't collide with the original on the watch)
- Message keys resolve through the SDK-generated `MESSAGE_KEY_*` symbols; `settings_process_tuple` is an if/else chain because those symbols are extern variables, not compile-time constants
- `wscript` adds `-Wno-error -Wno-zero-length-bounds -Wno-cast-function-type` to tolerate legacy code under the modern compiler — these should be fixed properly over time (tracked in issues)
