#include <pebble.h>
#include "animation.h"

// Forward decl: the stopped handler is referenced by animation_slide_in
// before its definition below.
static void animation_slide_in_stopped(Animation *animation, bool finished, void *context);

void animation_slide_in(Layer *layer, int delay, direction_t direction) {
  // Guard: callers may fire during settings re-init after deinit, when the
  // layer no longer exists.
  if (layer == NULL || !animation_layer_valid(layer)) {
    return;
  }

  // Revived launch cascade: slides a layer in from off-screen. Each module
  // calls this at the end of its init with a staggered delay, so the face
  // assembles piece by piece when the watchface launches.
  static PropertyAnimation *s_property_animation;
  int startX = 0, startY = 0;

  GRect from_frame = layer_get_frame(layer);
  GRect to_frame = from_frame;

  if (direction == LEFT || direction == RIGHT) {
    startX = (direction == LEFT) ? 200 + from_frame.size.w : -200 - from_frame.size.w;
    from_frame.origin.x = startX;
  }
  else { // UP || DOWN
    startY = (direction == UP) ? 228 + from_frame.size.h : -228 - from_frame.size.h;
    from_frame.origin.y = startY;
  }

  layer_set_frame(layer, from_frame);
  layer_set_hidden(layer, false);

  s_property_animation = property_animation_create_layer_frame(layer, &from_frame, &to_frame);
  animation_set_duration((Animation *) s_property_animation, ANIM_DURATION);
  animation_set_delay((Animation *) s_property_animation, delay);
  animation_set_curve((Animation *) s_property_animation, AnimationCurveEaseInOut);
  // Auto-destroy when done: these are fire-and-forget launch animations.
  animation_set_handlers((Animation *) s_property_animation, (AnimationHandlers) {
    .stopped = animation_slide_in_stopped,
  }, NULL);

  animation_schedule((Animation *) s_property_animation);
}

static void animation_slide_in_stopped(Animation *animation, bool finished, void *context) {
  PropertyAnimation *prop_anim = (PropertyAnimation *) animation;
  property_animation_destroy(prop_anim);
}

// Layers not yet added to the window tree (or removed during teardown) would
// make layer_get_frame read garbage. layer_get_window() returns non-NULL only
// for layers actually attached to a window, which is exactly the check we want.
bool animation_layer_valid(Layer *layer) {
  return layer != NULL && layer_get_window(layer) != NULL;
}

TextLayer *text_layer_create_detailed(GRect frame, GColor background_color, GColor foreground_color, GTextAlignment alignment, GFont font) {
  TextLayer *text_layer = text_layer_create(frame);
  text_layer_set_background_color(text_layer, background_color);
  text_layer_set_text_color(text_layer, foreground_color);
  text_layer_set_text_alignment(text_layer, alignment);
  text_layer_set_font(text_layer, font);
  return text_layer;
}
