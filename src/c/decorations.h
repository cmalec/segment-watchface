#pragma once
#include <pebble.h>
#include "settings.h"
#include "helpers.h"

void decorations_settings_callback();
void decorations_layer_update_callback(Layer *my_layer, GContext* ctx);
void decorations_wr_outer_layer_update_callback(Layer *my_layer, GContext* ctx);
void decorations_init();
void decorations_deinit();
void decorations_toggle(bool is_obstructed);
// cond is the raw WMO code from the phone; the face maps it to an icon.
void decorations_set_weather(int8_t now, int8_t hi, int8_t lo, uint8_t cond);
void decorations_update_weather();
