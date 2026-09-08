#pragma once
#include <pebble.h>

// Pure helpers (no UI dependencies) — unit-testable on the host.
void duration_to_time(int duration_s, int *hours, int *minutes);
void format_commas(int n, char *out);
uint8_t hex_to_num (char h);
GColor color_helper(GColor color, uint8_t inverted);
