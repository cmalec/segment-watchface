#include <pebble.h>
#include "main.h"
#include "helpers.h"
#include "settings.h"
#include "bluetooth.h"
#include "health.h"
#include "battery.h"
#include "timedigits.h"
#include "decorations.h"
#include "fonts.h"
#include "window.h"
#include "background.h"
#include "unobstructed.h"
#include "weather.h"

AppTimer * started_timer =NULL;

void app_started_callback(void *data) {
  started_timer =NULL;
  appStarted = true;

  // Now that the app is settled, ask the phone for the day's hi/lo temps.
  // AppMessage is not persisted, so every watch launch needs a fresh send;
  // the phone serves from cache when fresh and fetches when stale. Retries
  // with backoff in case the phone-side JS isn't listening yet.
  if (!powerSaveEngaged) {
    weather_request_scheduled();
  }
}
void handle_init(void) {
  appStarted = false;

  setlocale(LC_ALL, "");

  settings_init();
  weather_init();
  window_init();
  fonts_init();
  background_init();
  decorations_init();
  unobstructed_init();
  timedigits_init();
  #if defined (PBL_HEALTH)
    // Registered unconditionally so the Health toggle applies at runtime;
    // health_init() itself no-ops on layers when Health is off.
    health_init();
  #endif
  if (!powerSaveEngaged) {
    battery_init();
    bluetooth_init();
  }

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "memfree %d", heap_bytes_free());

  //delay started flag. Prevent settings vibe during first few seconds (HOUR_UNIT included in settings update)
  started_timer = app_timer_register(4000, app_started_callback, NULL);
}

void handle_deinit(void) {
  if (started_timer!=NULL) {
    app_timer_cancel(started_timer);
  }
  settings_deinit();
  weather_deinit();
  background_deinit();
  fonts_deinit();
  timedigits_deinit();
  #if defined (PBL_HEALTH)
    health_deinit();
  #endif
  if (!powerSaveEngaged) {
    battery_deinit();
    bluetooth_deinit();
  }
  decorations_deinit();
  unobstructed_deinit();
  window_deinit();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
