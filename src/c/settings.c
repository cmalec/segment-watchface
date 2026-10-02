#include <pebble.h>
#include <stddef.h>
#include "settings.h"
#include "helpers.h"
#include "decorations.h"
#include "battery.h"
#include "weather.h"

SettingsChangeCallback settings_callbacks[SETTINGS_CALLBACKS_COUNT] = {NULL};

// Latest weather values from the phone, held until the row can draw them.
// INT8_MIN = not known yet (a real temperature never is).
static int8_t weather_pending_now = INT8_MIN;
static int8_t weather_pending_hi = INT8_MIN;
static int8_t weather_pending_lo = INT8_MIN;
// A raw WMO code from the phone. 0xFF is not a code, so the row maps it to
// WEATHER_COND_NONE and draws no icon until a real condition arrives. (0 would
// alias WMO code 0 = clear and paint a sunny icon over missing data.)
static uint8_t weather_pending_cond = 0xFF;

Settings global_settings;
GColor colors[COLORS_NUM];
GColor colorsSet1[COLORS_NUM];

bool appStarted = false;
bool powerSaveEngaged = false;
bool settingsBusy = false;

// Set by settings_process_tuple when an inbound message carried at least one
// settings key. Weather and phone-battery pushes land on their own layers
// directly, and must not re-run the settings cascade or write flash.
static bool settings_changed = false;

AppTimer * delayed_save =NULL;

void settings_register_callback(SettingsChangeCallback callback, SettingsCallback callbackIdentity){
    settings_callbacks[callbackIdentity] = callback;
}
void settings_unregister_callback(SettingsCallback callbackIdentity){
    settings_callbacks[callbackIdentity] = NULL;
}

bool setting_is_power_save(int8_t h, int8_t m){
  if (!global_settings.PowerSave) {
    return false;
  }
  int8_t tested= h*2+1;
  if (m>=30) {
    tested++;
  }
  if (global_settings.PS_End > global_settings.PS_Start) {
    return ( (tested >= global_settings.PS_Start) && (tested < global_settings.PS_End) );
  }
  else {
    return ( (tested >= global_settings.PS_Start) || (tested < global_settings.PS_End) );
  }
}

/*
 * Copy a user-supplied button label into the fixed-size field.
 *
 * Two constraints, both from the face: the label box holds LABEL_MAX glyphs of
 * Lucida 14, and the font is baked from a fixed character set - anything
 * outside it renders as a missing glyph. So the text is filtered to the
 * characters the font actually carries and cut at the cap. An empty result is
 * allowed: the label simply disappears.
 */
static void settings_copy_label(char *dst, const char *src) {
  if (src == NULL) {
    // A malformed message must not throw away the label already set.
    return;
  }
  size_t n = 0;
  for (; *src != '\0' && n < LABEL_MAX; src++) {
    char c = *src;
    bool printable = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') || c == ' ' || c == '-' ||
                     c == '/' || c == '\'' || c == '~' || c == '.' ||
                     c == ',' || c == ':' || c == '%';
    if (printable) {
      dst[n++] = c;
    }
  }
  dst[n] = '\0';
}

void settings_process_tuple(Tuple *new_tuple) {
  uint32_t key = new_tuple->key;
  // Weather and phone-battery keys land on their own layers directly; only the
  // rest are settings and may trigger the cascade + save in settings_inbox.
  if (key != WTEMP_HI_KEY && key != WTEMP_LO_KEY && key != WTEMP_NOW_KEY &&
      key != WCOND_KEY && key != PBATT_LEVEL_KEY) {
    settings_changed = true;
  }
  // NOTE: message keys are extern variables in the modern SDK, not integer
  // constants, so this must be an if/else chain rather than a switch.
  if (key == HEALTH_KEY) {
    global_settings.Health = new_tuple->value->uint8;
  }
  else if (key == BLINK_KEY) {
    global_settings.Blink = new_tuple->value->uint8;
  }
  else if (key == INVERT_KEY) {
    global_settings.Invert = new_tuple->value->uint8;
  }
  else if (key == BLUETOOTHVIBE_KEY) {
    global_settings.BluetoothVibe = new_tuple->value->uint8;
  }
  else if (key == BLUETOOTH_SHOW_KEY) {
    global_settings.BluetoothShow = new_tuple->value->uint8;
  }
  else if (key == BATTERY_ICON_ONLY_KEY) {
    global_settings.BatteryIconOnly = new_tuple->value->uint8;
  }
  else if (key == TEMP_UNIT_KEY) {
    global_settings.TempUnit = new_tuple->value->uint8;
    // Phone does the conversion; share the unit so it re-sends converted
    // temps immediately.
    weather_share_unit();
  }
  else if (key == DATE_FORMAT_KEY) {
    uint8_t fmt = new_tuple->value->uint8;
    if (fmt > DATE_FMT_MONTH_WEEKDAY_DD) {
      // Unknown formats fall back to the default rather than rendering as
      // whatever the low byte happened to be.
      fmt = DATE_FMT_MMDDYY;
    }
    global_settings.DateFmt = fmt;
  }
  else if (key == LABEL_BACK_KEY || key == LABEL_PREV_KEY || key == LABEL_NEXT_KEY) {
    char *dst = (key == LABEL_BACK_KEY) ? global_settings.LabelBack
              : (key == LABEL_PREV_KEY) ? global_settings.LabelPrev
                                        : global_settings.LabelNext;
    settings_copy_label(dst, new_tuple->value->cstring);
  }
  else if (key == HOURLYVIBE_KEY) {
    global_settings.HourlyVibe = new_tuple->value->uint8;
  }
  else if (key == BRANDING_MASK_KEY) {
    global_settings.BrandingMask = new_tuple->value->uint8;
  }
  else if (key == BATTERY_HIDE_KEY) {
    global_settings.BatteryHide = new_tuple->value->uint8;
  }
  else if (key == SECONDS_KEY) {
    global_settings.Seconds = new_tuple->value->uint8;
  }
  else if (key == SLEEP_READOUT_KEY) {
    global_settings.SleepReadout = new_tuple->value->uint8;
  }
  else if (key == LABEL_FONT_KEY) {
    uint8_t font = new_tuple->value->uint8;
    if (font >= LABEL_FONT_COUNT) {
      font = LABEL_FONT_VOLLAZEE;   // unknown choice: the face's own
    }
    global_settings.LabelFont = font;
  }
  else if (key == POWERSAVE_KEY) {
    global_settings.PowerSave = new_tuple->value->uint8;
  }
  else if (key == PS_START_KEY) {
    global_settings.PS_Start = new_tuple->value->uint8;
  }
  else if (key == PS_END_KEY) {
    global_settings.PS_End = new_tuple->value->uint8;
  }
  else if (key == WTEMP_HI_KEY || key == WTEMP_LO_KEY || key == WTEMP_NOW_KEY) {
    // Temperatures arrive as signed Celsius ints from the phone (already
    // converted to the user's unit). Each key lands on the row as it arrives.
    int8_t value = (int8_t)new_tuple->value->int8;
    if (key == WTEMP_HI_KEY) {
      weather_pending_hi = value;
    }
    else if (key == WTEMP_LO_KEY) {
      weather_pending_lo = value;
    }
    else {
      weather_pending_now = value;
    }
    APP_LOG(APP_LOG_LEVEL_INFO, "weather: got %d (now=%d hi=%d lo=%d)",
            (int)value, (int)weather_pending_now, (int)weather_pending_hi, (int)weather_pending_lo);
    decorations_set_weather(weather_pending_now, weather_pending_hi, weather_pending_lo,
                            weather_pending_cond);
    weather_request_cancel();  // data arrived; stop the retry loop
  }
  else if (key == WCOND_KEY) {
    weather_pending_cond = new_tuple->value->uint8;
    APP_LOG(APP_LOG_LEVEL_INFO, "weather: got cond=%d", (int)weather_pending_cond);
    decorations_set_weather(weather_pending_now, weather_pending_hi, weather_pending_lo,
                            weather_pending_cond);
  }
  else if (key == PBATT_LEVEL_KEY) {
    // Phone battery percentage (0-100) from PebbleKit JS.
    uint8_t pct = new_tuple->value->uint8;
    if (pct > 100) {
      pct = 100;
    }
    battery_set_phone_percent(pct);
  }
  else if (key == SET_KEY) {
    #ifdef PBL_COLOR
      uint8_t cnt = strlen(new_tuple->value->cstring ) >>1;
      if (cnt>COLORS_NUM) {
        cnt=COLORS_NUM;
      }
      for (int i=0;i<cnt;i++) {
        colorsSet1[i].argb= (hex_to_num(new_tuple->value->cstring[i*2]) << 4) +
                                       hex_to_num(new_tuple->value->cstring[i*2+1]);
      }
    #endif
  }
}

void update_settings() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "update_settings()");

  if (!settingsBusy && appStarted) {
    settingsBusy = true;
    // NOTE: TIMEDIGITS is in the callback array too, so iterating the whole
    // array covers it — the old explicit call before the loop ran it twice.
    for(int i = 0; i < SETTINGS_CALLBACKS_COUNT; i++) {
      if (settings_callbacks[i] != NULL) {
        settings_callbacks[i]();
      }
    }
    settingsBusy = false;
  }
}

// Strongly encouraged by the SDK: surface dropped/failed comms so a lost
// settings message is visible in logs instead of silently ignored.
static void settings_inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "settings: inbox dropped %d", (int)reason);
}

void settings_inbox(DictionaryIterator *iter, void *context) {
  settings_changed = false;
  Tuple *t = dict_read_first(iter);
  if(t) {
    settings_process_tuple(t);
  }
  while(t != NULL) {
    t = dict_read_next(iter);
    if(t) {
      settings_process_tuple(t);
    }
  }

  if (settings_changed) {
    settings_load_colorSet1();
    update_settings();

    // Delayed setting save instead of in exit. The delay allows the screen to
    // update before the save.
    if (delayed_save != NULL) {
      app_timer_cancel(delayed_save);
    }
    delayed_save= app_timer_register(100, settings_save, NULL);
  }
}


void settings_default_values() {
  global_settings.version = SETTINGS_VERSION;
  // Health ON by default: the feature is the watch's raison d'être and a
  // fresh install (new UUID = wiped persisted settings) should show steps.
  global_settings.Health = 1;
  global_settings.Blink = 1;
  global_settings.Invert = 0;
  global_settings.BluetoothVibe = 1;
  global_settings.HourlyVibe = 1;
  global_settings.BrandingMask = 0;
  global_settings.BatteryHide = 0;
  global_settings.Seconds = 0;
  // Sleep readout on: row 1 shows last night's sleep until the day's steps pass
  // HEALTH_STEP_MIN. Turning it off (for people who don't wear the watch to
  // sleep) makes row 1 show steps all day.
  global_settings.SleepReadout = 1;
  // Vollazee by default: the face it was designed around.
  global_settings.LabelFont = LABEL_FONT_VOLLAZEE;
  global_settings.PowerSave = 0;
  global_settings.PS_Start = 47;   //23:00
  global_settings.PS_End = 15;     //07:00
  // BT icon hidden by default: the phone-app connection state it shows is
  // rarely actionable, and a mostly-blue badge is visual noise.
  global_settings.BluetoothShow = 0;
  global_settings.BatteryIconOnly = 0;
  global_settings.TempUnit = 0; // Celsius
  // mm/dd/yy: what the face rendered before the format became a setting (it
  // used the locale's %D, which is this on the locales the face ships to).
  global_settings.DateFmt = DATE_FMT_MMDDYY;
  // Stock button wording; the labels are the user's to replace.
  snprintf(global_settings.LabelBack, sizeof(global_settings.LabelBack), "LIGHT");
  snprintf(global_settings.LabelPrev, sizeof(global_settings.LabelPrev), "PREV");
  snprintf(global_settings.LabelNext, sizeof(global_settings.LabelNext), "NEXT");
  colors[c_bg1] = GColorWhite;
  colors[c_bg2] = GColorBlack;
  colors[c_bg3] = GColorWhite;
  colors[c_bg4] = GColorBlack;

  colors[c_bi1] = GColorBlack;
  colors[c_bi2] = PBL_IF_COLOR_ELSE(GColorOrange, GColorBlack);
  colors[c_bi3] = PBL_IF_COLOR_ELSE(GColorRed, GColorBlack);
  colors[c_bi4] = PBL_IF_COLOR_ELSE(GColorRed, GColorBlack);

  colors[c_bl1] = PBL_IF_COLOR_ELSE(GColorDukeBlue, GColorWhite);
  colors[c_bl2] = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack);
  colors[c_bl3] = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
  colors[c_bl4] = GColorWhite;

  colors[c_d1]  = GColorWhite;
  colors[c_d2]  = GColorWhite;
  colors[c_d3]  = GColorWhite;
  colors[c_d4]  = GColorBlack;
  colors[c_d5]  = GColorWhite;
  colors[c_d6]  = GColorWhite;
  colors[c_d7]  = GColorWhite;
  colors[c_d8]  = GColorWhite;
  colors[c_d9]  = GColorWhite;

  colors[c_t1]  = GColorBlack;
  colors[c_t2]  = GColorBlack;
  colors[c_t3]  = GColorWhite;
  colors[c_t4]  = GColorBlack;

  memcpy(colorsSet1,colors,sizeof(colorsSet1));
}

void settings_load_colorSet1() {
  memcpy(colors, colorsSet1, sizeof(colors));
}

/*
 * Adopt a persisted blob of n bytes (0 = nothing stored on the watch).
 *
 * Fields are only ever appended to the Settings struct (see settings.h), so an
 * older blob's prefix still describes the same fields in the same order: copy
 * that prefix and leave the appended tail at the defaults that
 * settings_default_values() has already installed. Growing the struct used to
 * discard the whole blob instead, which reset every user's face on update.
 *
 * v5 also *removed* three bytes from the middle of the struct (the colour-set
 * switch fields); an older blob has them where the current layout has
 * BluetoothShow, so the head and the tail are copied around that hole. All
 * versioned layouts have the hole at the same offset.
 *
 * A blob from a *newer* layout is the one case where the bytes cannot be
 * interpreted, so it is ignored and the defaults stand.
 */
void settings_adopt_blob(const void *blob, int n) {
  const Settings *stored = blob;
  if (n <= 0) {
    return;
  }
  if (stored->version == SETTINGS_VERSION) {
    size_t len = ((size_t)n < sizeof(Settings)) ? (size_t)n : sizeof(Settings);
    memcpy(&global_settings, blob, len);
  }
  else if (stored->version < SETTINGS_VERSION) {
    size_t hole = offsetof(Settings, BluetoothShow);
    size_t head = ((size_t)n < hole) ? (size_t)n : hole;
    memcpy(&global_settings, blob, head);
    size_t src = hole + 3;   // where the old layout's tail starts
    if ((size_t)n > src) {
      size_t tail = (size_t)n - src;
      size_t room = sizeof(Settings) - hole;
      if (tail > room) {
        tail = room;
      }
      memcpy((char *)&global_settings + hole, (const char *)blob + src, tail);
    }
    global_settings.version = SETTINGS_VERSION;
  }
}

void settings_init() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "settings_init()");

  settings_default_values();
  if(persist_exists(SETTINGS_KEY)) {
    Settings stored;
    int n = persist_read_data(SETTINGS_KEY, &stored, sizeof(stored));
    settings_adopt_blob(&stored, n);
  }
  #ifdef PBL_COLOR
    if(persist_exists(COLORSET1_KEY)) {
      persist_read_data(COLORSET1_KEY, &colorsSet1, sizeof(colorsSet1));
    }
  #endif

  //set configuration according to time the CLOCK
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  settings_load_colorSet1();

  powerSaveEngaged = false;
  if (global_settings.PowerSave==1) {
    powerSaveEngaged = setting_is_power_save(tick_time->tm_hour,tick_time->tm_min);
  }

  // Register callbacks BEFORE app_message_open so no message is missed in the
  // window between open and registration (SDK-recommended order).
  app_message_register_inbox_received(settings_inbox);
  app_message_register_inbox_dropped(settings_inbox_dropped);
  // Inbound must hold the WHOLE settings save from the phone: pkjs sends all
  // keys in one dictionary — 17 numeric tuples (~8B each) plus BOTH 50-char
  // color sets (~60B each) ≈ 264B. Claiming less (the old 128B) made every
  // config save exceed the buffer and get dropped with APP_MSG_BUFFER_OVERFLOW
  // on device — toggles appeared to do nothing. Outbound stays small
  // (weather request / unit share = 1 byte).
  app_message_open(512, 128);
}

void settings_save(void *data) {
  delayed_save =NULL;
  persist_write_data(SETTINGS_KEY, &global_settings, sizeof(global_settings));
  #ifdef PBL_COLOR
  persist_write_data(COLORSET1_KEY, &colorsSet1, sizeof(colorsSet1));
  #endif
}

void settings_deinit() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "settings_deinit()");

  // no need to save on exit unless the save after setting update did not happen yet
  if (delayed_save !=NULL) {
    app_timer_cancel(delayed_save);
    settings_save(NULL);
  }
}
