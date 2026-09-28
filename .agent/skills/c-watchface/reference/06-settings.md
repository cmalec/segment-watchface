# Part 6 — User Settings (Clay + persistent storage)

**Goal:** user-chosen background/text colors, date toggle, °C/°F unit — edited on the phone via Clay, persisted on the watch.

## 1. Settings struct + persistence

```c
#define SETTINGS_KEY 1

typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor TextColor;
  bool TemperatureUnit;   // false = Celsius, true = Fahrenheit
  bool ShowDate;
} ClaySettings;

static ClaySettings settings;

static void prv_default_settings() {
  settings.BackgroundColor = GColorBlack;
  settings.TextColor = GColorWhite;
  settings.TemperatureUnit = false;
  settings.ShowDate = true;
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_load_settings() {
  prv_default_settings();                                        // defaults first…
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));   // …then overwrite with saved data
}

// first line of init():
prv_load_settings();
```

Defaults-then-overwrite is what keeps a struct that gains new fields working with old saved blobs.

## 2. Apply settings to the UI

```c
static void prv_update_display() {
  window_set_background_color(s_main_window, settings.BackgroundColor);

  text_layer_set_text_color(s_time_layer, settings.TextColor);
  text_layer_set_text_color(s_date_layer, settings.TextColor);
  text_layer_set_text_color(s_weather_layer, settings.TextColor);

  layer_set_hidden(text_layer_get_layer(s_date_layer), !settings.ShowDate);
  layer_mark_dirty(s_battery_layer);          // the bar border uses TextColor
}
```

Call it at the end of `main_window_load()`, and from the inbox callback after settings change. In `battery_update_proc`, use `settings.TextColor` for the stroke and as the monochrome fallback.

## 3. Install Clay and declare keys

```bash
pebble package install @rebble/clay     # adds "@rebble/clay": "^1.0.8" to dependencies
```

`package.json`:

```json
"capabilities": ["location", "configurable"],
"messageKeys": ["TEMPERATURE", "CONDITIONS", "REQUEST_WEATHER",
                "BackgroundColor", "TextColor", "TemperatureUnit", "ShowDate"]
```

`configurable` is what puts the gear icon next to the watchface in the phone app.

## 4. Clay config page (`src/pkjs/config.js`)

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
      { "type": "toggle", "messageKey": "TemperatureUnit", "label": "Use Fahrenheit", "defaultValue": false },
      { "type": "toggle", "messageKey": "ShowDate",        "label": "Show Date",     "defaultValue": true }
  ]},
  { "type": "submit", "defaultValue": "Save Settings" }
];
```

Each `messageKey` must match `package.json → messageKeys`.

`src/pkjs/index.js` — the three lines go at the top, before the weather code:

```javascript
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);     // handles showConfiguration / webviewClosed itself
```

## 5. Handle both message types in one inbox

```c
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // --- weather (part 4)
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  if (temp_tuple && conditions_tuple) {
    static char temperature_buffer[8];
    static char weather_layer_buffer[42];
    int temp_value = (int)temp_tuple->value->int32;

    if (settings.TemperatureUnit) {
      temp_value = (temp_value * 9 / 5) + 32;
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°F", temp_value);
    } else {
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", temp_value);
    }

    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s",
             temperature_buffer, conditions_tuple->value->cstring);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }

  // --- Clay settings
  Tuple *bg_color_t = dict_find(iterator, MESSAGE_KEY_BackgroundColor);
  if (bg_color_t) settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);

  Tuple *text_color_t = dict_find(iterator, MESSAGE_KEY_TextColor);
  if (text_color_t) settings.TextColor = GColorFromHEX(text_color_t->value->int32);

  Tuple *temp_unit_t = dict_find(iterator, MESSAGE_KEY_TemperatureUnit);
  if (temp_unit_t) settings.TemperatureUnit = temp_unit_t->value->int32 == 1;

  Tuple *show_date_t = dict_find(iterator, MESSAGE_KEY_ShowDate);
  if (show_date_t) settings.ShowDate = show_date_t->value->int32 == 1;

  if (bg_color_t || text_color_t || temp_unit_t || show_date_t) {
    prv_save_settings();
    prv_update_display();

    if (temp_unit_t) {                     // stored value is Celsius: refetch for the new unit
      prv_request_weather();
    }
  }
}
```

Weather messages carry `TEMPERATURE`/`CONDITIONS`; Clay messages carry the setting keys. Checking which keys are present is all the discrimination needed.

**Increase the AppMessage buffers** so the larger Clay dictionary fits:

```c
app_message_open(256, 256);
```

## Verify

```bash
pebble build && pebble install --emulator emery
pebble emu-app-config --emulator emery        # opens the Clay page in a browser
```

Headless alternative (keys are the generated integer ids in `build/src/message_keys.auto.c`, alphabetical from 10000):

```bash
grep MESSAGE_KEY_ build/src/message_keys.auto.c          # Template: BackgroundColor=10003, ShowDate=10006, TemperatureUnit=10005, TextColor=10004
pebble send-app-message --int 10003=4144 10006=0 10005=1 --emulator emery
pebble screenshot --no-open --emulator emery shot-06.png  # bg=0x001030, date hidden, °F
```

Persistence: reinstall (`pebble install --emulator emery`) and confirm the face still shows the saved colors/toggle state — verified in the emulator, where settings survived a reinstall.

## Gotchas

- Keys live in three places and must agree: `package.json → messageKeys`, `config.js` `messageKey`, and the `MESSAGE_KEY_*` constants in C.
- Clay sends colors as int32 `0x00RRGGBB` → `GColorFromHEX()`. Toggles arrive as `1`/`0`.
- Change the temperature unit → the stored Celsius value must be refetched, otherwise the face shows the old number under the new unit.
- Every setting that affects a layer needs the redraw (`prv_update_display()`), including `layer_mark_dirty()` for custom-drawn layers.
- On monochrome platforms a user-picked color maps to black/white; that is expected.

Source: <https://developer.repebble.com/tutorials/watchface-tutorial/part6/>
