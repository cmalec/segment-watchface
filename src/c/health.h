#pragma once
#include <pebble.h>
#include "settings.h"

void health_settings_callback();
void health_update();
void health_init();
void health_deinit();
int16_t health_bpm_text_offset_x();
#ifdef PBL_PLATFORM_EMERY
void health_layout_row();
#endif
