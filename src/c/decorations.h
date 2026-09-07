#pragma once
#include <pebble.h>
#include "settings.h"

void decorations_settings_callback();
void decorations_layer_update_callback(Layer *my_layer, GContext* ctx);
void decorations_wr_outer_layer_update_callback(Layer *my_layer, GContext* ctx);
void decorations_init();
void decorations_deinit();
void decorations_toggle(bool is_obstructed);
void decorations_set_temps(int8_t hi, int8_t lo);
void decorations_update_temp_layers();
