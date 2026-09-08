#pragma once
#include <pebble.h>

#define SETTINGS_KEY 1336
#define SETTINGS_VERSION 3   // bump when the Settings struct layout changes
#define COLORSET1_KEY 1341
#define COLORSET2_KEY 1342

#define SETTINGS_CALLBACKS_COUNT 7
#define COLORS_NUM 25

typedef enum SettingsCallback {
  SETTINGS_CALLBACK_FONT = 0,
  SETTINGS_CALLBACK_BATTERY = 1,
  SETTINGS_CALLBACK_BLUETOOTH = 2,
  SETTINGS_CALLBACK_DECORATIONS = 3,
  SETTINGS_CALLBACK_TIMEDIGITS = 4,
  SETTINGS_CALLBACK_BACKGROUND = 5,
  SETTINGS_CALLBACK_HEALTH = 6
} SettingsCallback;

enum {
  c_bg1, c_bg2, c_bg3, c_bg4, //background
  c_bi1, c_bi2, c_bi3, c_bi4, //battery
  c_bl1, c_bl2, c_bl3, c_bl4, //bluetooth
  c_d1, c_d2, c_d3, c_d4, c_d5, c_d6, c_d7, c_d8, c_d9, //decorations
  c_t1, c_t2, c_t3, c_t4 //time digits
 };

typedef struct Settings {
  uint8_t version;  // = SETTINGS_VERSION; guards against reading a stale/short blob
  uint8_t Health;
  uint8_t Blink;
  uint8_t Invert;
  uint8_t BluetoothVibe;
  uint8_t HourlyVibe;
  uint8_t BrandingMask;
  uint8_t BatteryHide;
  uint8_t Seconds;
  uint8_t PowerSave;
  uint8_t PS_Start;
  uint8_t PS_End;
  uint8_t SwitchSet; //0 inactive, 1-time, 2-tap
  uint8_t SwitchStart;
  uint8_t SwitchEnd;
  // Appended fields only: persist reads the blob by size, so older saves
  // without these bytes still load (they just keep the defaults below).
  uint8_t BluetoothShow;
  uint8_t BatteryIconOnly; // 0 = icon + percent, 1 = percent only (no icon, no phone bar)
  uint8_t TempUnit;        // 0 = Celsius, 1 = Fahrenheit
} __attribute__((__packed__)) Settings;

extern Settings global_settings;
extern GColor colors[COLORS_NUM];
extern int8_t selectedSet;

typedef void (*SettingsChangeCallback)();

#define BLINK_KEY          MESSAGE_KEY_blink
#define INVERT_KEY         MESSAGE_KEY_invert
#define BLUETOOTHVIBE_KEY  MESSAGE_KEY_bluetoothvibe
#define BLUETOOTH_SHOW_KEY MESSAGE_KEY_bluetoothshow
#define BATTERY_ICON_ONLY_KEY MESSAGE_KEY_battery_icon_only
#define TEMP_UNIT_KEY      MESSAGE_KEY_temp_unit
#define HOURLYVIBE_KEY     MESSAGE_KEY_hourlyvibe
#define BRANDING_MASK_KEY  MESSAGE_KEY_branding_mask
#define BATTERY_HIDE_KEY   MESSAGE_KEY_battery_hide
#define SECONDS_KEY        MESSAGE_KEY_seconds
#define POWERSAVE_KEY      MESSAGE_KEY_powersave
#define PS_START_KEY       MESSAGE_KEY_ps_start
#define PS_END_KEY         MESSAGE_KEY_ps_end
#define SWITCHSET_KEY      MESSAGE_KEY_switchset
#define SWITCH_START_KEY   MESSAGE_KEY_switch_start
#define SWITCH_END_KEY     MESSAGE_KEY_switch_end
#define HEALTH_KEY         MESSAGE_KEY_health
#define WTEMP_REQ_KEY      MESSAGE_KEY_wtemp_req
#define WTEMP_HI_KEY       MESSAGE_KEY_wtemp_hi
#define WTEMP_LO_KEY       MESSAGE_KEY_wtemp_lo
#define PBATT_LEVEL_KEY    MESSAGE_KEY_pbatt_level

// Color-set payloads ride the messageKey values too (legacy hardcoded keys
// 200/202 predated the SDK-generated 10000+ range and matched nothing).
#define SET_KEY  MESSAGE_KEY_setcolors
#define SET2_KEY MESSAGE_KEY_set2colors


enum {
  BLINK_OFF = 0,
  BLINK_ON = 1,
  BLINK_DOUBLE_RATE = 2,
};


extern bool appStarted;
extern bool powerSaveEngaged;

bool setting_is_power_save(int8_t h, int8_t m);
bool setting_is_set2(int8_t h, int8_t m);
void update_settings();
void timed_colorset(int8_t h, int8_t m);
void settings_register_callback(SettingsChangeCallback callback, SettingsCallback callbackIdentity);
void settings_unregister_callback(SettingsCallback callbackIdentity);
void settings_process_tuple(Tuple *new_tuple);
void settings_inbox(DictionaryIterator *iter, void *context);
//Settings settings_get();
void settings_default_values();
void settings_load_colorSet1();
void settings_load_colorSet2();
void settings_save(void *data);
void settings_init();
void settings_deinit();
