/* Unit tests for src/c/settings.c range logic (power-save window) and the
 * settings wire/persist contracts. Host build. Pulls in the real settings.c
 * and stubs its UI/weather deps. */
#include "pebble.h"
#include "test_util.h"

/* ---- stubs for settings.c's external dependencies ---- */
int8_t seen_now = INT8_MIN, seen_hi = INT8_MIN, seen_lo = INT8_MIN;
uint8_t seen_cond = 0;
void decorations_set_weather(int8_t now, int8_t hi, int8_t lo, uint8_t cond) {
  seen_now = now; seen_hi = hi; seen_lo = lo; seen_cond = cond;
}
void battery_set_phone_percent(uint8_t pct) { (void)pct; }
void weather_request_cancel(void) {}
void weather_share_unit(void) {}
AppTimer *app_timer_register(uint32_t ms, AppTimerCallback cb, void *data) { (void)ms; (void)cb; (void)data; return NULL; }
bool app_timer_cancel(AppTimer *t) { (void)t; return true; }

/* settings.c defines these; declare to inspect */
#include "../../src/c/settings.h"

/* functions under test (settings.h) */
bool setting_is_power_save(int8_t h, int8_t m);
void settings_default_values(void);

/* helpers to drive the schedule window. Window = [start,end) in half-hour
 * indices 0..47. Non-wrapping: start<end. Wrapping: start>end. */
static void ps_window(int start, int end) {
  global_settings.PowerSave = 1;
  global_settings.PS_Start = start;
  global_settings.PS_End = end;
}

static void test_powersave_nonwrapping(void) {
  ps_window(2, 10);  // slots [2,10) = 01:00..04:59 (slot n covers n/2 h; end exclusive)
  ASSERT_TRUE( setting_is_power_save(0, 30), "00:30 -> slot 2 = start, inside");
  ASSERT_TRUE( setting_is_power_save(1, 0),  "01:00 -> slot 3, inside");
  ASSERT_TRUE(!setting_is_power_save(4, 59), "04:59 -> slot 10 = end, outside");
  ASSERT_TRUE(!setting_is_power_save(5, 0),  "05:00 -> slot 11, outside");
}

static void test_powersave_wrapping(void) {
  ps_window(47, 15); // 23:00-07:00
  ASSERT_TRUE( setting_is_power_save(23, 15), "23:15 inside");
  ASSERT_TRUE( setting_is_power_save(3, 0),   "03:00 inside");
  ASSERT_TRUE(!setting_is_power_save(12, 0),  "12:00 outside");
}

static void test_powersave_disabled(void) {
  global_settings.PowerSave = 0;
  ps_window(47, 15);
  global_settings.PowerSave = 0; // ps_window set it to 1; force off
  ASSERT_TRUE(!setting_is_power_save(23, 0), "disabled -> never in window");
}

/* The v3 layout, before DateFmt was appended: an older blob's prefix, with
 * the colour-set switch bytes v5 removed still in the middle. */
static struct __attribute__((__packed__)) V3Settings {
  uint8_t version, Health, Blink, Invert, BluetoothVibe, HourlyVibe,
          BrandingMask, BatteryHide, Seconds, PowerSave, PS_Start, PS_End,
          SwitchSet, SwitchStart, SwitchEnd, BluetoothShow, BatteryIconOnly,
          TempUnit;
} v3_blob;

/* The v4 layout: the current struct plus the removed switch bytes, and without
 * LabelFont (appended after v4). */
static struct __attribute__((__packed__)) V4Settings {
  uint8_t version, Health, Blink, Invert, BluetoothVibe, HourlyVibe,
          BrandingMask, BatteryHide, Seconds, PowerSave, PS_Start, PS_End,
          SwitchSet, SwitchStart, SwitchEnd, BluetoothShow, BatteryIconOnly,
          TempUnit, DateFmt;
  char LabelBack[LABEL_MAX + 1], LabelPrev[LABEL_MAX + 1], LabelNext[LABEL_MAX + 1];
  uint8_t SleepReadout;
} v4_blob;

/* Drive one tuple through settings_process_tuple, as the app message loop does.
 * Tuple carries a pointer to the value union; the mock mirrors the SDK's shape
 * closely enough that a void* assignment keeps the compiler quiet. */
static union {
  uint8_t uint8; int8_t int8; uint16_t uint16; int32_t int32; const char *cstring;
} wire_value;

static void send_uint(uint32_t key, uint8_t v) {
  wire_value.uint8 = v;
  Tuple tuple = { .key = key, .value = (void *)&wire_value };
  settings_process_tuple(&tuple);
}

static void send_cstring(uint32_t key, const char *text) {
  wire_value.cstring = text;
  Tuple tuple = { .key = key, .value = (void *)&wire_value };
  settings_process_tuple(&tuple);
}

static void test_date_format_tuple(void) {
  settings_default_values();
  send_uint(MESSAGE_KEY_date_format, DATE_FMT_WEEKDAY_DD);
  ASSERT_EQ(global_settings.DateFmt, DATE_FMT_WEEKDAY_DD, "valid format accepted");
  send_uint(MESSAGE_KEY_date_format, DATE_FMT_MONTH_WEEKDAY_DD);
  ASSERT_EQ(global_settings.DateFmt, DATE_FMT_MONTH_WEEKDAY_DD, "last format accepted");
  send_uint(MESSAGE_KEY_date_format, 200);
  ASSERT_EQ(global_settings.DateFmt, DATE_FMT_MMDDYY, "unknown format falls back to the default");
}

static void test_button_label_wire_contract(void) {
  settings_default_values();
  ASSERT_STR(global_settings.LabelBack, "LIGHT", "stock back label");

  send_cstring(MESSAGE_KEY_label_light, "MENU");
  ASSERT_STR(global_settings.LabelBack, "MENU", "label replaced");

  send_cstring(MESSAGE_KEY_label_light, "LONGER THAN EIGHT");
  ASSERT_STR(global_settings.LabelBack, "LONGER T", "label cut at the cap");

  send_cstring(MESSAGE_KEY_label_light, "a b#c\x80" "d");
  ASSERT_STR(global_settings.LabelBack, "a bcd", "unprintable glyphs dropped");

  send_cstring(MESSAGE_KEY_label_light, "");
  ASSERT_STR(global_settings.LabelBack, "", "empty label allowed (label hides)");

  send_cstring(MESSAGE_KEY_label_prev, "UP");
  ASSERT_STR(global_settings.LabelPrev, "UP", "prev label independent");
  ASSERT_STR(global_settings.LabelBack, "", "other labels untouched");

  send_cstring(MESSAGE_KEY_label_next, NULL);
  ASSERT_STR(global_settings.LabelNext, "NEXT", "null payload leaves the label alone");
}

static void test_sleep_readout_wire_contract(void) {
  settings_default_values();
  ASSERT_EQ(global_settings.SleepReadout, 1, "sleep readout on by default");

  send_uint(MESSAGE_KEY_sleep_readout, 0);
  ASSERT_EQ(global_settings.SleepReadout, 0, "turning it off lands");

  send_uint(MESSAGE_KEY_sleep_readout, 1);
  ASSERT_EQ(global_settings.SleepReadout, 1, "turning it back on lands");
}

static void test_label_font_wire_contract(void) {
  settings_default_values();
  ASSERT_EQ(global_settings.LabelFont, LABEL_FONT_VOLLAZEE, "labels default to the bundled face");

  send_uint(MESSAGE_KEY_label_font, LABEL_FONT_ROBOTO_CONDENSED);
  ASSERT_EQ(global_settings.LabelFont, LABEL_FONT_ROBOTO_CONDENSED, "a system face lands");

  send_uint(MESSAGE_KEY_label_font, LABEL_FONT_COUNT + 7);
  ASSERT_EQ(global_settings.LabelFont, LABEL_FONT_VOLLAZEE, "an unknown face falls back");
}

static void test_weather_wire_contract(void) {
  seen_now = seen_hi = seen_lo = INT8_MIN;
  seen_cond = 0;

  send_uint(MESSAGE_KEY_wtemp_hi, (uint8_t)28);
  ASSERT_EQ(seen_hi, 28, "high lands on the row");
  ASSERT_EQ(seen_lo, INT8_MIN, "low still unknown");

  send_uint(MESSAGE_KEY_wtemp_lo, (uint8_t)12);
  ASSERT_EQ(seen_lo, 12, "low lands on the row");
  ASSERT_EQ(seen_hi, 28, "high is remembered");

  send_uint(MESSAGE_KEY_wtemp_now, (uint8_t)19);
  ASSERT_EQ(seen_now, 19, "current lands on the row");
  ASSERT_EQ(seen_hi, 28, "high survives a later key");
  ASSERT_EQ(seen_lo, 12, "low survives a later key");

  send_uint(MESSAGE_KEY_wcond, 61);
  ASSERT_EQ(seen_cond, 61, "condition code lands on the row");
  ASSERT_EQ(seen_now, 19, "temperature survives the condition");
}

static void test_blob_migration_from_older_layout(void) {
  memset(&v3_blob, 0, sizeof(v3_blob));
  v3_blob.version = 3;
  v3_blob.Health = 0;          // user turned health off
  v3_blob.Seconds = 1;         // ...and seconds on
  v3_blob.SwitchSet = 99;      // removed bytes must not land anywhere
  v3_blob.BatteryIconOnly = 1;
  v3_blob.TempUnit = 1;
  v3_blob.PS_Start = 30;

  settings_default_values();
  settings_adopt_blob(&v3_blob, sizeof(v3_blob));

  ASSERT_EQ(global_settings.Health, 0, "older blob: health kept");
  ASSERT_EQ(global_settings.Seconds, 1, "older blob: seconds kept");
  ASSERT_EQ(global_settings.BatteryIconOnly, 1, "older blob: battery mode kept");
  ASSERT_EQ(global_settings.TempUnit, 1, "older blob: temp unit kept");
  ASSERT_EQ(global_settings.PS_Start, 30, "older blob: power-save window kept");
  ASSERT_EQ(global_settings.DateFmt, DATE_FMT_MMDDYY, "appended field keeps its default");
  // A save from before the setting existed must not turn the sleep readout off:
  // the missing byte leaves the default in place.
  ASSERT_EQ(global_settings.SleepReadout, 1, "older blob: sleep readout default stands");
  ASSERT_EQ(global_settings.version, SETTINGS_VERSION, "blob adopted as current");
}

static void test_blob_migration_skips_the_removed_switch_bytes(void) {
  memset(&v4_blob, 0, sizeof(v4_blob));
  v4_blob.version = 4;
  v4_blob.SwitchSet = 99;      // values the v5 layout has no fields for
  v4_blob.SwitchStart = 88;
  v4_blob.SwitchEnd = 77;
  v4_blob.Health = 0;
  v4_blob.BluetoothShow = 1;   // sits right after the removed bytes
  v4_blob.BatteryIconOnly = 1;
  v4_blob.TempUnit = 1;
  v4_blob.DateFmt = DATE_FMT_WEEKDAY_DD;
  snprintf(v4_blob.LabelBack, sizeof(v4_blob.LabelBack), "MENU");
  v4_blob.SleepReadout = 0;

  settings_default_values();
  settings_adopt_blob(&v4_blob, sizeof(v4_blob));

  ASSERT_EQ(global_settings.Health, 0, "v4 blob: health kept");
  ASSERT_EQ(global_settings.BluetoothShow, 1, "v4 blob: field after the hole kept");
  ASSERT_EQ(global_settings.BatteryIconOnly, 1, "v4 blob: battery mode kept");
  ASSERT_EQ(global_settings.TempUnit, 1, "v4 blob: temp unit kept");
  ASSERT_EQ(global_settings.DateFmt, DATE_FMT_WEEKDAY_DD, "v4 blob: date format kept");
  ASSERT_STR(global_settings.LabelBack, "MENU", "v4 blob: button label kept");
  ASSERT_EQ(global_settings.SleepReadout, 0, "v4 blob: sleep readout kept");
  ASSERT_EQ(global_settings.LabelFont, LABEL_FONT_VOLLAZEE,
            "field appended after v4 keeps its default");
  ASSERT_EQ(global_settings.version, SETTINGS_VERSION, "v4 blob adopted as current");
}

static void test_blob_migration_ignores_newer_layout(void) {
  uint8_t future[8];
  memset(future, 0, sizeof(future));
  future[0] = SETTINGS_VERSION + 1;

  settings_default_values();
  global_settings.Seconds = 1;               // current state to protect
  settings_adopt_blob(future, sizeof(future));

  ASSERT_EQ(global_settings.Seconds, 1, "newer blob: current state untouched");
  ASSERT_EQ(global_settings.version, SETTINGS_VERSION, "newer blob: version untouched");
}

static void test_blob_migration_of_current_and_empty_blobs(void) {
  Settings current;
  settings_default_values();
  current = global_settings;
  current.Seconds = 1;
  current.DateFmt = DATE_FMT_WEEKDAY_DD;
  settings_default_values();
  settings_adopt_blob(&current, sizeof(current));
  ASSERT_EQ(global_settings.Seconds, 1, "current blob: seconds restored");
  ASSERT_EQ(global_settings.DateFmt, DATE_FMT_WEEKDAY_DD, "current blob: date format restored");

  settings_default_values();
  settings_adopt_blob(&current, 0);          // nothing stored
  ASSERT_EQ(global_settings.Seconds, 0, "empty blob: defaults stand");
}

int main(void) {
  settings_default_values();  // establish a known baseline
  RUN(test_powersave_nonwrapping);
  RUN(test_powersave_wrapping);
  RUN(test_powersave_disabled);
  RUN(test_date_format_tuple);
  RUN(test_button_label_wire_contract);
  RUN(test_weather_wire_contract);
  RUN(test_sleep_readout_wire_contract);
  RUN(test_label_font_wire_contract);
  RUN(test_blob_migration_from_older_layout);
  RUN(test_blob_migration_skips_the_removed_switch_bytes);
  RUN(test_blob_migration_ignores_newer_layout);
  RUN(test_blob_migration_of_current_and_empty_blobs);
  TEST_SUMMARY();
}
