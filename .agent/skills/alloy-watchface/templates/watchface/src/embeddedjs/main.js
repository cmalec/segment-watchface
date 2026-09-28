// Alloy watchface — the end state of the official 6-part Alloy tutorial:
//   time + date (custom Jersey font) · battery bar · BT disconnect marker ·
//   Open-Meteo weather (hourly + 1 h cache) · Timeline Quick View aware ·
//   localStorage settings driven by a Clay config page on the phone.
//
// Targets: emery (200x228) and gabbro (260x260). See ../SKILL.md for the build/test loop.
//
// Structure rule: every module-scope binding is declared before any code that can
// invoke a callback (sensors, listeners). `let` bindings are in the temporal dead
// zone until their initializer runs, and a throw from a synchronous callback
// aborts module evaluation — "ReferenceError: get X: not initialized yet".

import Poco from "commodetto/Poco";
import parseBMF from "commodetto/parseBMF";
import parseRLE from "commodetto/parseRLE";
import Battery from "embedded:sensor/Battery";
import Location from "embedded:sensor/Location";
import Message from "pebble/message";

const render = new Poco(screen);

// ---------------------------------------------------------------- fonts

// Custom TTF loaded from the BMF/RLE resources declared in manifest.json.
// `size` must match a declared resource size exactly.
function getFont(name, size) {
    const font = parseBMF(new Resource(`${name}-${size}.fnt`));
    font.bitmap = parseRLE(new Resource(`${name}-${size}-alpha.bm4`));
    return font;
}

const timeFont = getFont("Jersey10-Regular", 56);
const dateFont = getFont("Jersey10-Regular", 24);
const smallFont = new render.Font("Gothic-Regular", 18);

// ---------------------------------------------------------------- settings

const DEFAULT_SETTINGS = {
    backgroundColor: { r: 0, g: 0, b: 0 },
    textColor: { r: 255, g: 255, b: 255 },
    useFahrenheit: false,
    showDate: true,
    use24Hour: true
};

// Spread-merge keeps saved data from older versions forward compatible.
function loadSettings() {
    const stored = localStorage.getItem("settings");
    if (stored) {
        try {
            return { ...DEFAULT_SETTINGS, ...JSON.parse(stored) };
        } catch (e) {
            console.log("Failed to parse settings");
        }
    }
    return { ...DEFAULT_SETTINGS };
}

function saveSettings() {
    localStorage.setItem("settings", JSON.stringify(settings));
}

let settings = loadSettings();

let bgColor = render.makeColor(settings.backgroundColor.r, settings.backgroundColor.g, settings.backgroundColor.b);
let textColor = render.makeColor(settings.textColor.r, settings.textColor.g, settings.textColor.b);

function updateColors() {
    bgColor = render.makeColor(settings.backgroundColor.r, settings.backgroundColor.g, settings.backgroundColor.b);
    textColor = render.makeColor(settings.textColor.r, settings.textColor.g, settings.textColor.b);
}

const green = render.makeColor(0, 170, 0);
const yellow = render.makeColor(255, 170, 0);
const red = render.makeColor(255, 0, 0);

// ---------------------------------------------------------------- state

const DAYS = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
const MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

// Last date seen from a time event; battery/BT/Clay redraws have no event.
let lastDate = new Date();

let weather = null;          // { temp, conditions } or null while unavailable
let batteryPercent = 100;
let isConnected = true;

// ---------------------------------------------------------------- weather

// Open-Meteo WMO weather codes → short labels.
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

// Weather is expensive (phone link + network): cache it so startup is instant.
function loadCachedWeather() {
    const cached = localStorage.getItem("weather");
    const cachedTime = localStorage.getItem("weatherTime");

    if (cached && cachedTime) {
        const age = Date.now() - Number(cachedTime);
        if (age < 60 * 60 * 1000) {
            try {
                weather = JSON.parse(cached);
                console.log("Using cached weather");
                return true;
            } catch (e) {
                console.log("Failed to parse cached weather");
            }
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

// Requests go through the raw HTTPS client instead of fetch(): the pebbleproxy
// fetch() path can hard-crash the XS VM on some responses with
// "TypeError: cannot coerce undefined to object (in Headers.prototype.set)".
// It still travels through the same pebbleproxy phone-side shim.
const WEATHER_HOST = "api.open-meteo.com";
let client = null;

function requestJSON(path, onSuccess, onFail) {
    const chunks = [];
    let status = 0;
    let watchdog = setTimeout(() => fail("timeout"), 20000);

    function finish(callback, arg) {
        if (!watchdog) return;
        clearTimeout(watchdog);
        watchdog = null;
        callback(arg);
    }

    function fail(error) {
        try {
            client?.close();      // release the socket so a retry gets a fresh client
        } catch (e) {
            // already closed
        }
        client = null;
        finish(onFail, error);
    }

    try {
        client ??= new device.network.https.io({
            ...device.network.https, host: WEATHER_HOST, port: 443,
            onError(error) {
                fail(error);
            }
        });
        client.request({
            path,
            headersMask: ["content-length"],   // sidesteps the proxied-header parsing bug
            onHeaders(value) {
                status = value;
            },
            onReadable(count) {
                if (count) chunks.push(String.fromArrayBuffer(this.read()));
            },
            onDone() {
                if (status < 200 || status > 299) {
                    return finish(onFail, "http " + status);
                }
                try {
                    finish(onSuccess, JSON.parse(chunks.join("")));
                } catch (e) {
                    finish(onFail, e);
                }
            },
            onError(error) {
                fail(error);
            }
        });
    } catch (e) {
        fail(e);
    }
}

// The first fetch after launch can fail transiently even when connected;
// without a retry the face would sit on "Loading..." until the next hour.
let retryTimer = null;
let retryCount = 0;

function scheduleRetry() {
    if (retryTimer || retryCount >= 3) return;
    retryCount++;
    retryTimer = setTimeout(() => {
        retryTimer = null;
        requestLocation();
    }, 10000);
}

function fetchWeather(latitude, longitude) {
    const params = {
        latitude,
        longitude,
        current: "temperature_2m,weather_code"
    };

    // Let the API return the requested unit instead of converting on the watch.
    if (settings.useFahrenheit) {
        params.temperature_unit = "fahrenheit";
    }

    console.log("Fetching weather...");
    requestJSON("/v1/forecast?" + new URLSearchParams(params).toString(), (data) => {
        weather = {
            temp: Math.round(data.current.temperature_2m),
            conditions: getWeatherDescription(data.current.weather_code)
        };

        retryCount = 0;
        refreshInFlight = false;
        console.log("Weather: " + weather.temp + ", " + weather.conditions);
        saveWeather();
        drawScreen();
    }, (error) => {
        refreshFailed(error);
    });
}

// Location is a one-shot sensor with a MODULE-GLOBAL lock: while an instance is
// open, `new Location()` throws "single instance only", and an uncaught throw
// kills the app. A request that never completes therefore blocks every future
// request, so it is always released on sample, error, or timeout.
// Refreshes are also serialized: overlapping requests on the shared proxied
// transport are what corrupts the response stream (VM-killing TypeError).
let location = null;
let locationTimer = null;
let refreshInFlight = false;

function releaseLocation() {
    if (locationTimer) {
        clearTimeout(locationTimer);
        locationTimer = null;
    }
    const pending = location;
    location = null;
    try {
        pending?.close();
    } catch (e) {
        // already closed
    }
}

function refreshFailed(error) {
    console.log("Weather refresh failed: " + error);
    refreshInFlight = false;
    releaseLocation();
    scheduleRetry();
}

function requestLocation() {
    if (refreshInFlight) return;      // one refresh at a time

    try {
        refreshInFlight = true;
        location = new Location({
            onSample() {
                const sample = this.sample();
                console.log("Got location: " + sample.latitude + ", " + sample.longitude);
                releaseLocation();
                fetchWeather(sample.latitude, sample.longitude);
            },
            onError() {
                refreshFailed("location error");
            }
        });
        locationTimer = setTimeout(() => {
            refreshFailed("location timeout");
        }, 30000);
    } catch (e) {
        refreshFailed(e);
    }
}

// ---------------------------------------------------------------- drawing

function drawBatteryBar() {
    const barWidth = (render.unobstructed.width / 2) | 0;
    const barX = ((render.unobstructed.width - barWidth) / 2) | 0;
    const barY = render.unobstructed.height < 180 ? 6 : 20;
    const barHeight = 8;

    // Hollow bar: colored border, background inner, then the fill.
    render.fillRectangle(textColor, barX, barY, barWidth, barHeight);
    render.fillRectangle(bgColor, barX + 1, barY + 1, barWidth - 2, barHeight - 2);

    let barColor;
    if (batteryPercent <= 20) {
        barColor = red;
    } else if (batteryPercent <= 40) {
        barColor = yellow;
    } else {
        barColor = green;
    }

    const fillWidth = ((batteryPercent * (barWidth - 4)) / 100) | 0;
    render.fillRectangle(barColor, barX + 2, barY + 2, fillWidth, barHeight - 4);
}

function drawScreen(event) {
    const now = event?.date ?? lastDate;
    if (event?.date) lastDate = event.date;

    render.begin();
    // Background always covers the full screen; content uses the unobstructed area.
    render.fillRectangle(bgColor, 0, 0, render.width, render.height);

    // Quick View can change the visible height at any time → compute per draw.
    const blockHeight = timeFont.height + dateFont.height;
    const timeY = (render.unobstructed.height - blockHeight) / 2;
    const dateY = timeY + timeFont.height;

    drawBatteryBar();

    if (!isConnected) {
        const btWidth = render.getTextWidth("X", smallFont);
        const btY = render.unobstructed.height < 180 ? 16 : 30;
        render.drawText("X", smallFont, red, (render.unobstructed.width - btWidth) / 2, btY);
    }

    let hours = now.getHours();
    if (!settings.use24Hour) {
        hours = hours % 12 || 12;
    }
    const timeStr = `${String(hours).padStart(2, "0")}:${String(now.getMinutes()).padStart(2, "0")}`;

    let width = render.getTextWidth(timeStr, timeFont);
    render.drawText(timeStr, timeFont, textColor, (render.unobstructed.width - width) / 2, timeY);

    if (settings.showDate) {
        const dateStr = `${DAYS[now.getDay()]} ${MONTHS[now.getMonth()]} ${String(now.getDate()).padStart(2, "0")}`;
        width = render.getTextWidth(dateStr, dateFont);
        render.drawText(dateStr, dateFont, textColor, (render.unobstructed.width - width) / 2, dateY);
    }

    const weatherY = render.unobstructed.height - smallFont.height -
        (render.unobstructed.height < 180 ? 6 : 20);
    const unit = settings.useFahrenheit ? "F" : "C";
    const weatherStr = weather ? `${weather.temp}°${unit} ${weather.conditions}` : "Loading...";
    width = render.getTextWidth(weatherStr, smallFont);
    render.drawText(weatherStr, smallFont, textColor, (render.unobstructed.width - width) / 2, weatherY);

    render.end();
}

// ---------------------------------------------------------------- side effects
// Everything below runs at startup and can invoke drawScreen from callbacks, so
// it comes after all declarations and function definitions.

loadCachedWeather();

const battery = new Battery({
    onSample() {
        batteryPercent = this.sample().percent;
        drawScreen();
    }
});
batteryPercent = battery.sample().percent;

function checkConnection() {
    isConnected = watch.connected.app;
    drawScreen();
}
watch.addEventListener("connected", checkConnection);
checkConnection();

// Time listeners fire immediately on registration — that is the initial draw/fetch.
watch.addEventListener("minutechange", drawScreen);
watch.addEventListener("hourchange", requestLocation);
watch.addEventListener("resize", drawScreen);

// Settings arrive from the Clay page as AppMessages.
const message = new Message({
    keys: ["BackgroundColor", "TextColor", "TemperatureUnit", "ShowDate", "HourFormat"],
    onReadable() {
        const msg = this.read();

        // Clay sends colors as int32 0x00RRGGBB and toggles as 1/0.
        const bg = msg.get("BackgroundColor");
        if (bg !== undefined) {
            settings.backgroundColor = { r: (bg >> 16) & 0xFF, g: (bg >> 8) & 0xFF, b: bg & 0xFF };
        }
        const tc = msg.get("TextColor");
        if (tc !== undefined) {
            settings.textColor = { r: (tc >> 16) & 0xFF, g: (tc >> 8) & 0xFF, b: tc & 0xFF };
        }
        const tu = msg.get("TemperatureUnit");
        if (tu !== undefined) {
            settings.useFahrenheit = tu === 1;
        }
        const sd = msg.get("ShowDate");
        if (sd !== undefined) {
            settings.showDate = sd === 1;
        }
        const hf = msg.get("HourFormat");
        if (hf !== undefined) {
            settings.use24Hour = hf === 1;
        }

        saveSettings();
        updateColors();
        drawScreen();

        if (tu !== undefined) {
            requestLocation();   // unit changed → refetch so the value matches
        }
    }
});
