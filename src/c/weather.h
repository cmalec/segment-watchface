#pragma once
#include <pebble.h>

// Phone-side weather request plumbing.
// weather_init() registers outbox callbacks; call once from handle_init
// before any request. weather_request_scheduled() fires a request with slow
// retries until temps arrive or the budget is spent; weather_request_cancel()
// stops the loop when temps land. weather_deinit() cleans timers.
void weather_init(void);
void weather_deinit(void);
void weather_request(void);
void weather_request_scheduled(void);
void weather_request_cancel(void);
