#pragma once
#include <pebble.h>
#include "settings.h"

// Pure helpers (no UI dependencies) — unit-testable on the host.
void duration_to_time(int duration_s, int *hours, int *minutes);
void format_commas(int n, char *out);
void format_date(DateFormat fmt, struct tm *t, char *out, size_t out_len);

// Weather condition the face can draw, from the WMO code the phone sends.
// NONE means "nothing to show", which draws no icon. Each condition is a glyph
// of the weather-icons font (see decorations.c); COUNT is the glyph table's
// size, not a condition.
typedef enum WeatherCond {
  WEATHER_COND_NONE = 0,
  WEATHER_COND_CLEAR,
  WEATHER_COND_PARTLY_CLOUDY,
  WEATHER_COND_CLOUDY,
  WEATHER_COND_FOG,
  WEATHER_COND_DRIZZLE,
  WEATHER_COND_RAIN,
  WEATHER_COND_SHOWERS,
  WEATHER_COND_SLEET,
  WEATHER_COND_SNOW,
  WEATHER_COND_THUNDERSTORM,
  WEATHER_COND_COUNT
} WeatherCond;

WeatherCond weather_cond_from_wmo(uint8_t code);
uint8_t hex_to_num (char h);
GColor color_helper(GColor color, uint8_t inverted);
