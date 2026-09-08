/* Unit tests for src/c/settings.c range logic (set2 window, power-save window).
 * Host build. Pulls in the real settings.c and stubs its UI/weather deps. */
#include "pebble.h"
#include "test_util.h"

/* ---- stubs for settings.c's external dependencies ---- */
void decorations_set_temps(int8_t hi, int8_t lo) { (void)hi; (void)lo; }
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

int main(void) {
  settings_default_values();  // establish a known baseline
  RUN(test_set2_nonwrapping);
  RUN(test_set2_wrapping_overnight);
  RUN(test_set2_half_hour_resolution);
  RUN(test_powersave_nonwrapping);
  RUN(test_powersave_wrapping);
  RUN(test_powersave_disabled);
  TEST_SUMMARY();
}
