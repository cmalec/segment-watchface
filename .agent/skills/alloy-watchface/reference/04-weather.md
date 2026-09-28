# Part 4 — Weather via watch-side fetch() + Open-Meteo

**Goal:** temperature + conditions at the bottom of the face, refreshed hourly.

## How networking works in Alloy

The watch has no internet. `fetch()` on the watch is proxied through PebbleKit JS (PKJS) running on the phone:

```
Watch fetch() ──AppMessage──> PKJS pebbleproxy ──HTTP──> api.open-meteo.com
Watch Location sensor ──────> PKJS pebbleproxy ──GPS──> phone
```

Requirements: `@moddable/pebbleproxy` in the project, a phone (or the emulator's pypkjs) with internet.

## 1. Phone side (3 lines)

```bash
pebble package install @moddable/pebbleproxy     # or add "@moddable/pebbleproxy": "^0.1.5" to dependencies
```

`src/pkjs/index.js` — create it, and keep the four lines even when Clay is added later (Clay is prepended):

```javascript
const moddableProxy = require("@moddable/pebbleproxy");
Pebble.addEventListener('ready', moddableProxy.readyReceived);
Pebble.addEventListener('appmessage', moddableProxy.appMessageReceived);
```

The proxy forwards both `fetch()` and `Location` traffic. No custom GPS or HTTP code in PKJS.

## 2. Location capability

`package.json`:

```json
"capabilities": ["location"]
```

## 3. Location sensor (one-shot)

```javascript
import Location from "embedded:sensor/Location";

let location = null;                     // keep a reference so the instance is not collected

function requestLocation() {
    location = new Location({
        onSample() {
            const sample = this.sample();        // latitude/longitude as decimal degrees
            console.log("Got location: " + sample.latitude + ", " + sample.longitude);
            this.close();                        // ONE-SHOT: always close
            fetchWeather(sample.latitude, sample.longitude);
        }
    });
}
```

Wrap it in a function so each refresh gets fresh coordinates.

## 4. Fetch + parse

```javascript
function getWeatherDescription(code) {
    if (code === 0) return "Clear";
    if (code <= 3) return "Cloudy";
    if (code <= 48) return "Fog";
    if (code <= 55) return "Drizzle";
    if (code <= 57) return "Fz. Drizzle";
    if (code <= 65) return "Rain";
    if (code <= 67) return "Fz. Rain";
    if (code <= 75) return "Snow";
    if (code <= 77) return "Snow Grains";
    if (code <= 82) return "Showers";
    if (code <= 86) return "Snow Shwrs";
    if (code === 95) return "T-Storm";
    if (code <= 99) return "T-Storm";
    return "Unknown";
}

async function fetchWeather(latitude, longitude) {
    try {
        const url = new URL("https://api.open-meteo.com/v1/forecast");
        url.search = new URLSearchParams({
            latitude,
            longitude,
            current: "temperature_2m,weather_code"
        });
        const response = await fetch(url);
        const data = await response.json();
        weather = {
            temp: Math.round(data.current.temperature_2m),
            conditions: getWeatherDescription(data.current.weather_code)
        };
        drawScreen();
    } catch (e) {
        console.log("Weather fetch error: " + e);
        retryWeather();      // see below
    }
}
```

Open-Meteo is free, keyless, and returns clean JSON — no `XMLHttpRequest` or manual parsing callbacks.

## 5. Refresh schedule

```javascript
watch.addEventListener("hourchange", requestLocation);   // fires immediately → initial fetch too
```

## 6. Hardening (verified on SDK 4.33.1 — the tutorial's snippet alone is not safe)

### Location has a module-global lock

`new Location()` throws `Error: single instance only` while another instance is open, and an **uncaught throw kills the app** (`Alloy: Fatal Error ... at Location() at requestLocation() at onReadable()` observed in the emulator). A request that never produces a sample blocks every later request forever. Always:

```javascript
let location = null, locationTimer = null, refreshInFlight = false;

function releaseLocation() {
    if (locationTimer) { clearTimeout(locationTimer); locationTimer = null; }
    const pending = location;
    location = null;
    try { pending?.close(); } catch (e) {}
}

function requestLocation() {
    if (refreshInFlight) return;              // serialize
    try {
        refreshInFlight = true;
        location = new Location({
            onSample() { const s = this.sample(); releaseLocation(); fetchWeather(s.latitude, s.longitude); },
            onError() { refreshFailed("location error"); }        // clears flag, closes, retries
        });
        locationTimer = setTimeout(() => refreshFailed("location timeout"), 30000);
    } catch (e) { refreshFailed(e); }
}
```

`close()` is what releases the lock; `onError` and the watchdog are how you guarantee it happens.

### Proxied HTTP can abort the VM

Observed in the emulator: `TypeError: cannot coerce undefined to object (in Headers.prototype.set)` → app replaced by the Alloy fatal-error screen. It fires inside the proxied transport, **not catchable from app code**, and it appeared with both `fetch()` and the raw `device.network.https.io` client. Mitigations, in order:

1. **Serialize requests** — one weather refresh in flight at a time (the `refreshInFlight` guard above). Overlapping responses on the shared AppMessage transport are the trigger.
2. **Retry with a watchdog** (below) so a dropped response doesn't strand the face on "Loading…".
3. If it still bites, do the HTTP on the phone instead: `XMLHttpRequest` in `src/pkjs/index.js` (the C tutorial's pattern, see `pebble-watchface/templates/pkjs-weather.js`) and ship the result to the watch as a string over a `Message` key. The watch then never touches the proxied HTTP parser.

The template ships the raw `device.network.https.io` client (with `headersMask: ["content-length"]`) plus serialization; the `fetch()` form shown above is the tutorial's teaching version and is fine for a quick start.

## 7. Transient-failure retry

The first fetch after launch can fail even when connected. Without a retry, the face can sit on "Loading…" for an hour:

```javascript
let retryTimer = null;
let retryCount = 0;

function scheduleRetry() {
    if (retryTimer || retryCount >= 3) return;
    retryCount++;
    retryTimer = setTimeout(() => { retryTimer = null; requestLocation(); }, 10000);
}
```

Reset `retryCount = 0` on a successful fetch; route every failure path (fetch error, location error, location timeout) through the same `refreshFailed()` so the in-flight flag and the sensor lock are always released before the retry.

## 8. Display

```javascript
const weatherY = render.height - smallFont.height - (render.height < 180 ? 6 : 20);
const str = weather ? `${weather.temp}°C ${weather.conditions}` : "Loading...";
const w = render.getTextWidth(str, smallFont);
render.drawText(str, smallFont, white, (render.width - w) / 2, weatherY);
```

## Verify

```bash
pebble build && pebble install --emulator emery
pebble screenshot --no-open --emulator emery shot-04.png    # read the image
```

The emulator's PKJS uses the host's network and geo, so a real value appears at the bottom of the face (e.g. `27°C Cloudy`, `81°F Cloudy`). To prove the value came from the network and not the cache, clear app data (`pebble wipe`, which prompts), reinstall, then compare a screenshot taken a few seconds in with one taken ~20 s in. `pebble logs` is unreliable in some emulator setups — an empty log file proves nothing, so verify visually.

## Gotchas

- Forgetting `close()` leaks a live Location request and blocks every future one (`single instance only` → app death). Release on sample, error, **and** timeout.
- `capabilities: ["location"]` is required or GPS silently never resolves.
- Don't parse weather codes inline in the draw path; map once at fetch time.
- Never leave two refreshes in flight: the shared proxy transport is where the `Headers.prototype.set` abort comes from.
- Reading `watch.connected.app` before fetching avoids pointless attempts when the phone link is down.

Source: <https://developer.repebble.com/tutorials/alloy-watchface-tutorial/part4/>
