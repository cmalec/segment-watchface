# Part 6 — User Settings (localStorage + Clay)

**Goal:** user-chosen background/text colors, date toggle, 12/24 h format, °C/°F unit — persisted on the watch, edited from a phone-side Clay page; plus a cached weather value so startup is instant.

## 1. Settings model + persistence

```javascript
const DEFAULT_SETTINGS = {
    backgroundColor: { r: 0, g: 0, b: 0 },
    textColor: { r: 255, g: 255, b: 255 },
    useFahrenheit: false,
    showDate: true,
    use24Hour: true
};

function loadSettings() {
    const stored = localStorage.getItem("settings");
    if (stored) {
        try {
            return { ...DEFAULT_SETTINGS, ...JSON.parse(stored) };   // old data + new fields
        } catch (e) { console.log("Failed to parse settings"); }
    }
    return { ...DEFAULT_SETTINGS };
}
function saveSettings() { localStorage.setItem("settings", JSON.stringify(settings)); }

let settings = loadSettings();
```

`localStorage` stores strings only → `JSON.stringify`/`parse`. The spread merge is the Alloy equivalent of loading C struct defaults first: new settings added later still get values from old saved blobs.

## 2. Colors from settings

```javascript
let bgColor = render.makeColor(settings.backgroundColor.r, settings.backgroundColor.g, settings.backgroundColor.b);
let textColor = render.makeColor(settings.textColor.r, settings.textColor.g, settings.textColor.b);

function updateColors() {
    bgColor = render.makeColor(settings.backgroundColor.r, settings.backgroundColor.g, settings.backgroundColor.b);
    textColor = render.makeColor(settings.textColor.r, settings.textColor.g, settings.textColor.b);
}
```

Call `updateColors()` after any settings change; use `bgColor`/`textColor` everywhere (including the battery bar border) instead of hardcoded black/white.

## 3. Apply the toggles

```javascript
let hours = now.getHours();
if (!settings.use24Hour) hours = hours % 12 || 12;      // 0 → 12
const timeStr = `${String(hours).padStart(2, "0")}:${String(now.getMinutes()).padStart(2, "0")}`;

if (settings.showDate) { /* date drawing block */ }

const unit = settings.useFahrenheit ? "F" : "C";
const weatherStr = `${weather.temp}°${unit} ${weather.conditions}`;
```

Temperature unit is forwarded to the API (no on-watch conversion):

```javascript
if (settings.useFahrenheit) params.temperature_unit = "fahrenheit";
```

## 4. Weather cache

```javascript
function loadCachedWeather() {
    const cached = localStorage.getItem("weather");
    const cachedTime = localStorage.getItem("weatherTime");
    if (cached && cachedTime) {
        const age = Date.now() - Number(cachedTime);
        if (age < 60 * 60 * 1000) {                 // 1 h TTL
            try { weather = JSON.parse(cached); return true; } catch (e) {}
        }
    }
    return false;
}

function saveWeather() {
    if (weather) {
        localStorage.setItem("weather", JSON.stringify(weather));
        localStorage.setItem("weatherTime", String(Date.now()));
    }
}
```

Call `loadCachedWeather()` at startup and `saveWeather()` after a successful fetch: the face shows last hour's weather instead of "Loading…".

## 5. Clay (phone-side settings page)

```bash
pebble package install @rebble/clay      # or "@rebble/clay": "^1.0.8"
```

`package.json`:

```json
"capabilities": ["location", "configurable"],
"messageKeys": ["BackgroundColor", "TextColor", "TemperatureUnit", "ShowDate", "HourFormat"]
```

`configurable` is what makes the gear icon appear next to the face in the phone app. `messageKeys` are the AppMessage channel names.

`src/pkjs/config.js` — JSON schema; each `messageKey` must match `package.json`:

```javascript
module.exports = [
  { "type": "heading", "defaultValue": "Watchface Settings" },
  { "type": "text", "defaultValue": "Customize your watchface appearance and preferences." },
  { "type": "section", "items": [
      { "type": "heading", "defaultValue": "Colors" },
      { "type": "color",  "messageKey": "BackgroundColor", "defaultValue": "0x000000", "label": "Background Color" },
      { "type": "color",  "messageKey": "TextColor",       "defaultValue": "0xFFFFFF", "label": "Text Color" }
  ]},
  { "type": "section", "items": [
      { "type": "heading", "defaultValue": "Preferences" },
      { "type": "toggle", "messageKey": "TemperatureUnit", "label": "Use Fahrenheit",       "defaultValue": false },
      { "type": "toggle", "messageKey": "ShowDate",        "label": "Show Date",           "defaultValue": true },
      { "type": "toggle", "messageKey": "HourFormat",      "label": "Use 24-Hour Format",  "defaultValue": true }
  ]},
  { "type": "submit", "defaultValue": "Save Settings" }
];
```

Clay handles `showConfiguration`/`webviewClosed` itself. `src/pkjs/index.js` — Clay first, proxy after (different event types, no conflict):

```javascript
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

const moddableProxy = require("@moddable/pebbleproxy");
Pebble.addEventListener('ready', moddableProxy.readyReceived);
Pebble.addEventListener('appmessage', moddableProxy.appMessageReceived);
```

## 6. Receive on the watch

```javascript
import Message from "pebble/message";

const message = new Message({
    keys: ["BackgroundColor", "TextColor", "TemperatureUnit", "ShowDate", "HourFormat"],
    onReadable() {
        const msg = this.read();

        const bg = msg.get("BackgroundColor");
        if (bg !== undefined) settings.backgroundColor = { r: (bg >> 16) & 0xFF, g: (bg >> 8) & 0xFF, b: bg & 0xFF };
        const tc = msg.get("TextColor");
        if (tc !== undefined) settings.textColor = { r: (tc >> 16) & 0xFF, g: (tc >> 8) & 0xFF, b: tc & 0xFF };
        const tu = msg.get("TemperatureUnit");
        if (tu !== undefined) settings.useFahrenheit = tu === 1;
        const sd = msg.get("ShowDate");
        if (sd !== undefined) settings.showDate = sd === 1;
        const hf = msg.get("HourFormat");
        if (hf !== undefined) settings.use24Hour = hf === 1;

        saveSettings();
        updateColors();
        drawScreen();
        if (tu !== undefined) requestLocation();   // unit changed → refetch weather
    }
});
```

Clay sends colors as int32 `0x00RRGGBB` (unpack with `>> 16 & 0xFF`, `>> 8 & 0xFF`, `& 0xFF`) and toggles as `1`/`0`.

## Verify

```bash
pebble build && pebble install --emulator emery
pebble emu-app-config --emulator emery       # opens the Clay page
```

Change colors/toggles → Save → the face redraws immediately. To drive the watch-side path without a browser, send a Clay-shaped AppMessage from the emulator CLI — keys are the generated integer ids from `build/js/message_keys.json` (alphabetical, starting at 10000):

```bash
pebble send-app-message --int 10000=4144 10003=0 10002=1 --emulator emery   # bg=0x001030, hide date, °F
pebble screenshot --no-open --emulator emery shot-06.png
```

Persistence: relaunch the app (`pebble install` again) and confirm the face still uses the saved colors/toggles — verified in the emulator, where settings survived a reinstall.

## Gotchas

- Three places must agree on every key: `package.json` `messageKeys`, `config.js` `messageKey`, `Message` `keys`.
- An **uncaught exception inside `onReadable` kills the app** (observed: a re-entrant `requestLocation()` throwing `single instance only` took the face down). Keep the handler defensive and route refetches through a serialized refresh function.
- Colors in `localStorage` are `{r,g,b}` objects; colors over AppMessage are int32 — keep the conversion at the boundary.
- Recreate Poco color values (`updateColors()`) after loading settings — the renderer's color objects are not live views of the settings object.
- Clay's phone-side config is identical for C and Alloy; only the watch-side receive path differs (`Message` vs `dict_find`).

Source: <https://developer.repebble.com/tutorials/alloy-watchface-tutorial/part6/>
