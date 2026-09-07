#include <pebble.h>
#include "weather.h"
#include "settings.h"

// Ask the phone for the day's hi/lo temps. The phone answers from its cache
// when fresh, or fetches anew when stale.
//
// Error 64 (APP_MSG_BUSY) seen in the field: a request that never gets acked
// (phone JS not running / phone offline) leaves the outbox occupied, and
// blind retries just stack up BUSY results. So retries are conservative:
// few, widely spaced, and never sent while a previous send is still in
// flight (outbox_busy guards that).

static bool s_outbox_busy = false;   // a send is awaiting its callback
static AppTimer *s_retry_timer = NULL;
static int s_retry_count = 0;

#define WEATHER_RETRY_MAX 4
#define WEATHER_RETRY_DELAY_MS 15000

void weather_request(void) {
  if (s_outbox_busy) {
    APP_LOG(APP_LOG_LEVEL_INFO, "weather req: outbox busy, skipping");
    return;
  }
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "weather req: begin failed %d", (int)result);
    return;
  }
  dict_write_uint8(iter, WTEMP_REQ_KEY, 1);
  s_outbox_busy = true;
  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    s_outbox_busy = false;
    APP_LOG(APP_LOG_LEVEL_WARNING, "weather req: send failed %d", (int)result);
  }
}

// AppMessage delivery callbacks (registered in settings.c alongside the
// inbox handler). Clear the busy flag on either outcome so retries can go.
static void weather_out_sent(DictionaryIterator *iter, void *context) {
  s_outbox_busy = false;
}
static void weather_out_failed(DictionaryIterator *iter, AppMessageResult result, void *context) {
  s_outbox_busy = false;
  APP_LOG(APP_LOG_LEVEL_WARNING, "weather req: delivery failed %d", (int)result);
}

static void weather_retry_callback(void *data) {
  s_retry_timer = NULL;
  if (s_retry_count >= WEATHER_RETRY_MAX) {
    return;
  }
  s_retry_count++;
  weather_request();
  if (s_retry_timer == NULL) {
    s_retry_timer = app_timer_register(WEATHER_RETRY_DELAY_MS, weather_retry_callback, NULL);
  }
}

// Fire a request; keep retrying at a slow cadence until temps arrive or the
// retry budget is spent. Call weather_request_cancel() when temps land.
void weather_request_scheduled(void) {
  s_retry_count = 0;
  weather_request();
  if (s_retry_timer == NULL) {
    s_retry_timer = app_timer_register(WEATHER_RETRY_DELAY_MS, weather_retry_callback, NULL);
  }
}

// Call when temps actually arrive to stop the retry loop.
void weather_request_cancel(void) {
  s_retry_count = WEATHER_RETRY_MAX;  // stop the loop without touching timers in flight
  if (s_retry_timer != NULL) {
    app_timer_cancel(s_retry_timer);
    s_retry_timer = NULL;
  }
}

void weather_init(void) {
  app_message_register_outbox_sent(weather_out_sent);
  app_message_register_outbox_failed(weather_out_failed);
}

void weather_deinit(void) {
  if (s_retry_timer != NULL) {
    app_timer_cancel(s_retry_timer);
    s_retry_timer = NULL;
  }
}
