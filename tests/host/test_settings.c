/* Unit tests for src/c/settings.c range logic (set2 window, power-save window).
 * Host build. Pulls in the real settings.c and stubs its UI/weather deps. */
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
void accel_tap_service_subscribe(void *h) { (void)h; }
void accel_tap_service_unsubscribe(void) {}
AppTimer *app_timer_register(uint32_t ms, AppTimerCallback cb, void *data) { (void)ms; (void)cb; (void)data; return NULL; }
bool app_timer_cancel(AppTimer *t) { (void)t; return true; }

/* settings.c defines these; declare to inspect */
#include "../../src/c/settings.h"

/* functions under test (settings.h) */
bool setting_is_set2(int8_t h, int8_t m);
bool setting_is_power_save(int8_t h, int8_t m);
void settings_default_values(void);

/* helpers to drive the schedule window. Window = [start,end) in half-hour
 * indices 0..47. Non-wrapping: start<end. Wrapping: start>end. */
static void set2_window(int start, int end) {
  global_settings.SwitchStart = start;
  global_settings.SwitchEnd = end;
}
static void ps_window(int start, int end) {
  global_settings.PowerSave = 1;
  global_settings.PS_Start = start;
  global_settings.PS_End = end;
}

static void test_set2_nonwrapping(void) {
  // window 06:00-12:00 -> half-hour slots [13,24) i.e. 06:00..11:59 (end slot exclusive)
  set2_window(13, 24);
  ASSERT_TRUE(!setting_is_set2(5, 0),  "05:00 -> slot 11, outside");
  ASSERT_TRUE(!setting_is_set2(5, 59), "05:59 -> slot 12, outside");
  ASSERT_TRUE( setting_is_set2(6, 0),  "06:00 -> slot 13, inside (start)");
  ASSERT_TRUE(!setting_is_set2(11, 59),"11:59 -> slot 24 = end, outside");
  ASSERT_TRUE(!setting_is_set2(12, 0), "12:00 -> slot 25, outside");
}

static void test_set2_wrapping_overnight(void) {
  // window 23:00-07:00 -> indices 47..15 (wraps midnight)
  set2_window(47, 15);
  ASSERT_TRUE( setting_is_set2(23, 0),  "23:00 inside");
  ASSERT_TRUE( setting_is_set2(23, 59), "23:59 inside");
  ASSERT_TRUE( setting_is_set2(0, 0),   "00:00 inside (after midnight)");
  ASSERT_TRUE( setting_is_set2(6, 59),  "06:59 inside");
  ASSERT_TRUE(!setting_is_set2(7, 0),   "07:00 outside (end)");
  ASSERT_TRUE(!setting_is_set2(12, 0),  "12:00 outside");
}

static void test_set2_half_hour_resolution(void) {
  // minute >=30 bumps to the next slot: tested = h*2+1, +1 when m>=30
  set2_window(13, 24);
  ASSERT_TRUE(!setting_is_set2(5, 29), "05:29 -> slot 11, outside");
  ASSERT_TRUE(!setting_is_set2(5, 30), "05:30 -> slot 12, outside");
  ASSERT_TRUE( setting_is_set2(6, 30), "06:30 -> slot 14, inside");
  ASSERT_TRUE(!setting_is_set2(11, 30),"11:30 -> slot 24 = end, outside");
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

/* The layout as it was before DateFmt was appended: an older blob's prefix. */
static struct __attribute__((__packed__)) OldSettings {
  uint8_t version, Health, Blink, Invert, BluetoothVibe, HourlyVibe,
          BrandingMask, BatteryHide, Seconds, PowerSave, PS_Start, PS_End,
          SwitchSet, SwitchStart, SwitchEnd, BluetoothShow, BatteryIconOnly,
          TempUnit;
} old_blob;

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
  memset(&old_blob, 0, sizeof(old_blob));
  old_blob.version = SETTINGS_VERSION - 1;
  old_blob.Health = 0;          // user turned health off
  old_blob.Seconds = 1;         // ...and seconds on
  old_blob.BatteryIconOnly = 1;
  old_blob.TempUnit = 1;
  old_blob.PS_Start = 30;

  settings_default_values();
  settings_adopt_blob(&old_blob, sizeof(old_blob));

  ASSERT_EQ(global_settings.Health, 0, "older blob: health kept");
  ASSERT_EQ(global_settings.Seconds, 1, "older blob: seconds kept");
  ASSERT_EQ(global_settings.BatteryIconOnly, 1, "older blob: battery mode kept");
  ASSERT_EQ(global_settings.TempUnit, 1, "older blob: temp unit kept");
  ASSERT_EQ(global_settings.PS_Start, 30, "older blob: power-save window kept");
  ASSERT_EQ(global_settings.DateFmt, DATE_FMT_MMDDYY, "appended field keeps its default");
  ASSERT_EQ(global_settings.version, SETTINGS_VERSION, "blob adopted as current");
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
  RUN(test_set2_nonwrapping);
  RUN(test_set2_wrapping_overnight);
  RUN(test_set2_half_hour_resolution);
  RUN(test_powersave_nonwrapping);
  RUN(test_powersave_wrapping);
  RUN(test_powersave_disabled);
  RUN(test_date_format_tuple);
  RUN(test_button_label_wire_contract);
  RUN(test_weather_wire_contract);
  RUN(test_blob_migration_from_older_layout);
  RUN(test_blob_migration_ignores_newer_layout);
  RUN(test_blob_migration_of_current_and_empty_blobs);
  TEST_SUMMARY();
}
