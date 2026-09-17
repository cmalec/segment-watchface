#pragma once
#include <pebble.h>
#include "settings.h"

// Pure helpers (no UI dependencies) — unit-testable on the host.
void duration_to_time(int duration_s, int *hours, int *minutes);
void format_commas(int n, char *out);
void format_date(DateFormat fmt, struct tm *t, char *out, size_t out_len);

// Weather condition the face can draw, from the WMO code the phone sends.
// NONE means "nothing to show", which draws no icon.
typedef enum WeatherCond {
  WEATHER_COND_NONE = 0,
  WEATHER_COND_CLEAR,
  WEATHER_COND_CLOUD,
  WEATHER_COND_RAIN,
  WEATHER_COND_SNOW
} WeatherCond;

WeatherCond weather_cond_from_wmo(uint8_t code);
uint8_t hex_to_num (char h);
GColor color_helper(GColor color, uint8_t inverted);
