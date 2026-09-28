---
name: alloy-watchface
description: Build and extend Pebble watchfaces in Alloy (watch-side JavaScript on Moddable XS) following the official 6-part Alloy watchface tutorial — Poco rendering, custom TTF fonts, battery + Bluetooth indicators, Open-Meteo weather via watch-side fetch and pebbleproxy, Timeline Quick View, and localStorage + Clay settings. Use for any Pebble Alloy/JS watchface task, new or existing.
---

# Alloy Watchface (JavaScript)

Alloy = JS running **on the watch** (Moddable XS engine). Marked by `"projectType": "moddable"` in `package.json`.
**Targets: `emery` (200x228, default) and `gabbro` (260x260). Nothing else — other platforms require C** (use the `c-watchface` skill for the native-SDK path; general C APIs live in the `pebble-watchface` skill).

Everything in this skill is verified against pebble tool v5.0.40 / SDK 4.33.1 (emery emulator).

## Fast path — complete watchface from the verified template

```bash
SKILL=<this skill dir>
cp -R "$SKILL/templates/watchface" my-face && cd my-face
uuidgen | tr 'A-Z' 'a-z'                    # paste into package.json → pebble.uuid
python3 "$SKILL/scripts/fetch_font.py" .    # Jersey10-Regular.ttf → src/embeddedjs/assets/
pebble build                                # auto-installs declared Pebble packages
pebble install --emulator emery
pebble screenshot --no-open --emulator emery shot.png   # then READ the image
```

The template is the tutorial's end state: time + date (custom Jersey font), battery bar, BT disconnect marker, Open-Meteo weather with 1 h cache, Quick View adaptation, localStorage settings, Clay config page. It builds clean for emery and gabbro and was verified running in the emery emulator (see "Verified behaviour" below).

### Verified behaviour (emery emulator, SDK 4.33.1)

| Check | Observed |
|---|---|
| Boot + render | Jersey time/date, green battery bar, weather appears within ~6 s of a fresh install |
| Live weather | `27°C Cloudy` with an empty cache; `81°F Cloudy` after switching the unit (API called with `temperature_unit=fahrenheit`) |
| Battery | red fill at 10–15 %, green at 60 % |
| Bluetooth | red `X` under the bar while disconnected |
| Timeline Quick View | time/date/weather compress into the unobstructed area, restore on `off` |
| Settings message | Clay-shaped AppMessage applied colors, hid the date, switched to 12 h/°F and refetched weather — then still alive and redrawing |
| Persistence | settings survived app relaunch/reinstall (`localStorage`) |

## Project anatomy

```
project/
├── package.json               # projectType moddable, watchapp.watchface:true, capabilities, messageKeys
├── wscript                    # stock Pebble wscript — never edit
└── src/
    ├── c/mdbl.c               # 156-byte VM boot stub — never edit
    ├── embeddedjs/
    │   ├── main.js            # ALL watch-side logic
    │   ├── manifest.json      # modules + font resources (`*-alpha`)
    │   └── assets/            # .ttf files
    └── pkjs/
        ├── index.js           # phone side: pebbleproxy (networking) + Clay (settings)
        └── config.js          # Clay config schema (settings UI)
```

Scaffold alternative: `pebble new-project --alloy <name>` (gives part 1 of the tutorial; still set `watchapp.watchface`).

## Build / test loop

| Goal | Command |
|---|---|
| Build PBW | `pebble build` → `build/<name>.pbw` |
| Install + boot emulator | `pebble install --emulator emery` |
| Round variant | `pebble install --emulator gabbro` |
| Screenshot (**read the PNG afterwards**) | `pebble screenshot --no-open --emulator emery shot.png` |
| Logs (`console.log` from main.js/pkjs) | `pebble logs --emulator emery` — **may stay empty in some setups; never treat an empty log as failure** |
| Battery state | `pebble emu-battery --percent 15 --emulator emery` |
| Disconnect / reconnect | `pebble emu-bt-connection --connected no --emulator emery` (can take a few seconds; re-issue if the screenshot is unchanged) |
| Timeline Quick View | `pebble emu-set-timeline-quick-view on --emulator emery` |
| Settings page | `pebble emu-app-config --emulator emery` (opens a browser); headless alternative in [reference/06-settings.md](reference/06-settings.md) |
| Clock / 12h mode | `pebble emu-set-time ...`, `pebble emu-time-format --format 12h` |
| Clear app data | `pebble wipe` — **prompts interactively** |
| Reset wedged emulator | `pebble kill && pebble wipe` |

Verification is not optional: install, screenshot, and visually confirm the layout (no clipping, elements centered, requested features present). A blank/white screen at launch means the JS VM died — check your system-font name+size combination first (see Pitfalls).

## Part map (tutorial → reference docs)

| Part | Topic | Reference |
|---|---|---|
| 1 | Poco renderer, time + date, `minutechange` | [reference/01-time-and-date.md](reference/01-time-and-date.md) |
| 2 | Custom TTF fonts, centered layout, precomputed metrics | [reference/02-custom-fonts.md](reference/02-custom-fonts.md) |
| 3 | Battery sensor, battery bar, BT disconnect, event-less redraws | [reference/03-battery-and-connection.md](reference/03-battery-and-connection.md) |
| 4 | pebbleproxy, Location sensor, `fetch()` + Open-Meteo, hourly refresh | [reference/04-weather.md](reference/04-weather.md) |
| 5 | Timeline Quick View via `render.unobstructed.*` + `resize` | [reference/05-timeline-quick-view.md](reference/05-timeline-quick-view.md) |
| 6 | localStorage settings, weather cache, Clay config + `Message` | [reference/06-settings.md](reference/06-settings.md) |

Upstream source: <https://github.com/coredevices/alloy-watchface-tutorial> (per-part working projects).
API deep-dive for Alloy generally: `pebble-watchface` skill → `reference/alloy-guide.md`.

## Core patterns

### Poco frame

```javascript
import Poco from "commodetto/Poco";
const render = new Poco(screen);            // `screen` is a global

function drawScreen(event) {
    const now = event?.date ?? lastDate;    // redraws from battery/BT events have no event
    if (event?.date) lastDate = event.date;

    render.begin();
    render.fillRectangle(bgColor, 0, 0, render.width, render.height);  // full screen
    // ... draw ...
    render.end();
}
```

Draw primitives: `fillRectangle`, `drawText(str, font, color, x, y)`, `getTextWidth(str, font)`, `drawLine`, `drawCircle`, `drawRoundRect`, `frameRoundRect`, `drawBitmap`. Fonts expose `.height`. `render.width/height` = screen; `render.unobstructed.width/height` = area not covered by Quick View.

### Events

```javascript
watch.addEventListener("minutechange", drawScreen);  // fires IMMEDIATELY on registration → initial draw
watch.addEventListener("hourchange", requestLocation);// same: immediate → initial weather fetch
watch.addEventListener("connected", checkConnection); // watch.connected.app: boolean
watch.addEventListener("resize", drawScreen);         // Quick View appeared/disappeared
```

Time listeners fire on registration — do **not** add a separate startup draw. Never use `secondchange` for a minute display (battery).

### Custom fonts

```javascript
import parseBMF from "commodetto/parseBMF";
import parseRLE from "commodetto/parseRLE";
function getFont(name, size) {
    const font = parseBMF(new Resource(`${name}-${size}.fnt`));
    font.bitmap = parseRLE(new Resource(`${name}-${size}-alpha.bm4`));
    return font;
}
const timeFont = getFont("Jersey10-Regular", 56);
```

Declared in `manifest.json` (`resources."*-alpha"`), TTF in `src/embeddedjs/assets/`, sizes must match the declaration exactly.

### Sensors

```javascript
import Battery from "embedded:sensor/Battery";      // keep open
const battery = new Battery({ onSample() { batteryPercent = this.sample().percent; drawScreen(); } });
batteryPercent = battery.sample().percent;          // initial read

import Location from "embedded:sensor/Location";    // ONE-SHOT: close after read
new Location({ onSample() { const s = this.sample(); this.close(); fetchWeather(s.latitude, s.longitude); } });
```

### Networking (watch-side `fetch`)

Phone-side proxy, 3 lines, plus `"capabilities": ["location"]` for GPS:

```javascript
const moddableProxy = require("@moddable/pebbleproxy");
Pebble.addEventListener('ready', moddableProxy.readyReceived);
Pebble.addEventListener('appmessage', moddableProxy.appMessageReceived);
```

```javascript
const url = new URL("https://api.open-meteo.com/v1/forecast");
url.search = new URLSearchParams({ latitude, longitude, current: "temperature_2m,weather_code" });
const data = await (await fetch(url)).json();
```

Wrap in try/catch, cache the result, and retry with a watchdog — the first fetch right after launch can fail transiently even when connected.

**Two hard-won rules (verified on SDK 4.33.1, see [reference/04-weather.md](reference/04-weather.md)):**

1. `new Location()` is single-instance: it throws `single instance only` while a previous instance is open, and an uncaught throw kills the app. Release it on sample, **error, and timeout**, and serialize refreshes.
2. The proxied HTTP transport can abort the VM with `TypeError: cannot coerce undefined to object (in Headers.prototype.set)` — uncatchable, seen with both `fetch()` and raw `device.network.https.io`. Keep exactly one refresh in flight. If it persists, move the HTTP to `src/pkjs/index.js` (`XMLHttpRequest`, the C tutorial's pattern) and send the result to the watch as a string over a `Message` key.

The template ships the raw `device.network.https.io` client (with `headersMask: ["content-length"]`) plus the serialization/watchdog guards; `fetch()` remains the simpler teaching form.

### Persistence + settings

```javascript
const DEFAULT_SETTINGS = { /* ... */ };
function loadSettings() {
    const stored = localStorage.getItem("settings");
    if (stored) { try { return { ...DEFAULT_SETTINGS, ...JSON.parse(stored) }; } catch (e) {} }
    return { ...DEFAULT_SETTINGS };
}
```

Spread-merge keeps old saved data forward-compatible. Clay sends settings as AppMessage; watch side receives with `new Message({ keys: [...], onReadable() { const msg = this.read(); ... } })`. Keys must match `package.json → pebble.messageKeys` **and** `src/pkjs/config.js` `messageKey` values. Colors arrive as int32 `0xRRGGBB`; toggles as `1`/`0`.

## Pitfalls

| Symptom | Cause / fix |
|---|---|
| White screen, instant exit, no error in logs | Invalid system font name+size (e.g. `Bitham-Bold` at 48). Valid: Gothic-Regular/Bold 14/18/24/28, Bitham-Bold 42, Bitham-Black 30, Bitham-Light 42, Roboto-Condensed 21, Leco-Regular 20/26/28/32/36/38/42, Droid-Serif 28. Otherwise use a custom TTF. |
| "Module not found" at runtime | Extra JS modules must be listed in `manifest.json → modules`. |
| Text drifts / clipped when a notification appears | Position content with `render.unobstructed.*`, clear the background with `render.width/height`, recompute layout inside the draw function. |
| Layout frozen after first draw | Layout constants computed at module scope using screen size — recompute in `drawScreen()` (Quick View changes height). |
| Weather stuck on "Loading…" | Fetch failed once and refresh only happens hourly — retry with a watchdog. |
| Stale weather after restart | Cache in `localStorage` with a timestamp + TTL, load at startup. |
| App dies with `single instance only` at `Location()` | A previous Location request was never released — close on sample, error **and** timeout; serialize refreshes. |
| App dies with `cannot coerce undefined to object (in Headers.prototype.set)` | Proxied HTTP abort (uncatchable). One refresh in flight at a time; else move the request into pkjs `XMLHttpRequest` + `Message`. |
| App dies with `ReferenceError: get <x>: not initialized yet` | A callback fired during module evaluation, before that `let` binding's initializer ran — module init aborted. Declare all state first, instantiate sensors and register listeners **last**. |
| GPS never resolves | Missing `"capabilities": ["location"]`; Location sensor instance not closed after sampling. |
| Settings page gear missing | Missing `"configurable"` capability. |
| Settings don't arrive on watch | `messageKeys` in package.json ≠ `Message` `keys` array ≠ `config.js` `messageKey`s. |

## Delivery checklist

- [ ] `package.json`: `projectType: "moddable"`, `watchapp.watchface: true`, unique UUID, `targetPlatforms` ⊆ `[emery, gabbro]`
- [ ] `src/c/mdbl.c` + stock `wscript` untouched
- [ ] `main.js` draws between `begin()`/`end()`, uses `minutechange`, handles event-less redraws
- [ ] Custom fonts declared in `manifest.json` at every size used; TTF present in `assets/`
- [ ] `pebble build` clean → `build/<name>.pbw`
- [ ] Installed on emery, screenshot read and visually verified (no clipping, centered, features visible)
- [ ] Battery/BT/Quick View/settings paths exercised with the `emu-*` commands when implemented
