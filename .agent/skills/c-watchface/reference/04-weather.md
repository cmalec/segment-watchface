# Part 4 — Weather via PebbleKit JS + AppMessage

**Goal:** temperature + conditions at the bottom of the face, refreshed twice an hour from Open-Meteo.

## How it works

The watch has no internet. PebbleKit JS (PKJS) runs on the phone, does the HTTP and geolocation work, and ships the result to the watch over AppMessage:

```
Watch (C) ──AppMessage──> PKJS (index.js) ──XMLHttpRequest──> api.open-meteo.com
Watch (C) <──AppMessage── PKJS             <──geolocation──── phone GPS
```

## 1. Keys and capabilities

`package.json`:

```json
"capabilities": ["location"],
"messageKeys": ["TEMPERATURE", "CONDITIONS", "REQUEST_WEATHER"]
```

`location` lets the phone read GPS; the keys become `MESSAGE_KEY_*` constants in C. All three are needed — one to ask, two to answer.

## 2. Lay out the weather text

```c
static TextLayer *s_weather_layer;

int weather_y = bounds.size.h - PBL_IF_ROUND_ELSE(40, 30);
s_weather_layer = text_layer_create(GRect(0, weather_y, bounds.size.w, 25));
text_layer_set_background_color(s_weather_layer, GColorClear);
text_layer_set_text_color(s_weather_layer, GColorWhite);
text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
text_layer_set_text(s_weather_layer, "Loading...");
layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
```

## 3. AppMessage boilerplate

```c
static void inbox_received_callback(DictionaryIterator *iterator, void *context) { }
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// in init() — register BEFORE opening or early messages are lost:
app_message_register_inbox_received(inbox_received_callback);
app_message_register_inbox_dropped(inbox_dropped_callback);
app_message_register_outbox_failed(outbox_failed_callback);
app_message_register_outbox_sent(outbox_sent_callback);
app_message_open(128, 128);        // 256+ once Clay shares the channel (part 6)
```

## 4. Phone side (`src/pkjs/index.js`)

```javascript
var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () { callback(this.responseText); };
  xhr.open(type, url);
  xhr.send();
};

function weatherCodeToCondition(code) {
  if (code === 0) return 'Clear';
  if (code <= 3) return 'Cloudy';
  if (code <= 48) return 'Fog';
  if (code <= 55) return 'Drizzle';
  if (code <= 57) return 'Fz. Drizzle';
  if (code <= 65) return 'Rain';
  if (code <= 67) return 'Fz. Rain';
  if (code <= 75) return 'Snow';
  if (code <= 77) return 'Snow Grains';
  if (code <= 82) return 'Showers';
  if (code <= 86) return 'Snow Shwrs';
  if (code === 95) return 'T-Storm';
  if (code <= 99) return 'T-Storm';
  return 'Unknown';
}

function locationSuccess(pos) {
  var url = 'https://api.open-meteo.com/v1/forecast?' +
      'latitude=' + pos.coords.latitude +
      '&longitude=' + pos.coords.longitude +
      '&current=temperature_2m,weather_code';

  xhrRequest(url, 'GET', function (responseText) {
    var json = JSON.parse(responseText);
    Pebble.sendAppMessage({
      'TEMPERATURE': Math.round(json.current.temperature_2m),
      'CONDITIONS': weatherCodeToCondition(json.current.weather_code)
    }, function () { console.log('sent'); }, function () { console.log('send failed'); });
  });
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(locationSuccess,
    function (err) { console.log('Error requesting location!'); },
    { timeout: 15000, maximumAge: 60000 });
}

Pebble.addEventListener('ready', function () { getWeather(); });      // watchface opened
Pebble.addEventListener('appmessage', function (e) {
  if (e.payload['REQUEST_WEATHER']) getWeather();                     // watch asked
});
```

Open-Meteo is free and keyless, and the temperature always arrives in Celsius — the watch converts for Fahrenheit (part 6).

## 5. Watch side: parse and display

```c
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  if (temp_tuple && conditions_tuple) {
    static char temperature_buffer[8];
    static char weather_layer_buffer[42];

    snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", (int)temp_tuple->value->int32);
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s",
             temperature_buffer, conditions_tuple->value->cstring);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}
```

Numbers arrive as `int32`, strings as `cstring`.

## 6. Refresh loop

```c
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  if (tick_time->tm_min % 30 == 0) {            // twice an hour
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
      dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
      app_message_outbox_send();
    }
  }
}
```

## Verify

```bash
pebble build && pebble install --emulator emery
pebble screenshot --no-open --emulator emery shot-04.png    # read the image
```

The emulator's PKJS uses the host's network and geolocation, so a real value appears (e.g. `23°C Clear`) within seconds of launch. `pebble logs` is unreliable in some setups — an empty log proves nothing; verify visually.

## Gotchas

- Key names must match in three places: `package.json → messageKeys`, the C `MESSAGE_KEY_*` usage, and the pkjs dictionary keys.
- Check `app_message_outbox_begin()`'s result; sending while another message is in flight fails silently otherwise.
- The phone must have location permission and connectivity; `maximumAge` avoids a fresh GPS fix on every refresh.
- Watch-side buffers passed to `text_layer_set_text()` must be `static`.

Source: <https://developer.repebble.com/tutorials/watchface-tutorial/part4/>
