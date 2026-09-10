#pragma once
#include <pebble.h>
#include "_globals.h"

// Slide a layer in from off-screen (launch cascade). Fire-and-forget.
void animation_slide_in(Layer *layer, int delay, direction_t direction);
// True when the layer is attached to a window and safe to animate.
bool animation_layer_valid(Layer *layer);
// Convenience: create + fully configure a TextLayer.
TextLayer *text_layer_create_detailed(GRect frame, GColor background_color, GColor foreground_color, GTextAlignment alignment, GFont font);
