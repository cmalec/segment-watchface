#include <pebble.h>
#include <ctype.h>
#include "helpers.h"
#include "_globals.h"
#include "window.h"

// Forward decl: the stopped handler is referenced by animation_slide_in
// before its definition below.
static void animation_slide_in_stopped(Animation *animation, bool finished, void *context);

void duration_to_time(int duration_s, int *hours, int *minutes) {
  *hours = duration_s / 3600;
  *minutes = (duration_s % 3600) / 60;
}

void format_commas(int n, char *out)
{
    int c;
    char buf[20];
    char *p;

    snprintf(buf, sizeof(buf), "%d", n);
    c = 2 - strlen(buf) % 3;
    for (p = buf; *p != 0; p++) {
       *out++ = *p;
       if (c == 1) {
           *out++ = ',';
       }
       c = (c + 1) % 3;
    }
    *--out = 0;
}

char *upcase(char *str) {
  char *s = str;
  while (*s) {
    *s++ = toupper((int)*s);
  }
  return str;
}

uint8_t hex_to_num (char h){
  uint8_t rslt=0;
  if ((h>='0') && (h<='9'))
    rslt= h-'0';
  else if ((h>='A') && (h<='F'))
    rslt = h-'A'+10;
  else if ((h>='a') && (h<='f'))
    rslt = h-'a'+10;
  return rslt;
}

GColor color_inverted(GColor source) {
    GColor inverted = source;
  if(gcolor_equal(source, GColorBlack))
    inverted= GColorWhite;
  if(gcolor_equal(source, GColorWhite))
    inverted= GColorBlack;
  #ifdef PBL_COLOR
    if(!gcolor_equal(source, GColorClear)) //GColorClear should not change
      inverted.argb= source.argb ^ 0b00111111;
  #endif
  return inverted;
}


/*
static int animation_slot = 0;
static Animation animations[10];

void animation_slide_in_cleanup() {
     for(int i = 0; i < 10; i++) {
        property_animation_destroy(animations[i]);
    }
}
*/

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

TextLayer *text_layer_create_detailed(GRect frame, bool hidden, GColor background_color, GColor foreground_color, GTextAlignment alignment, GFont font) {
  TextLayer *text_layer = text_layer_create(frame);
  layer_set_hidden(text_layer_get_layer(text_layer), hidden);
  text_layer_set_background_color(text_layer, background_color);
  text_layer_set_text_color(text_layer, foreground_color);
  text_layer_set_text_alignment(text_layer, alignment);
  text_layer_set_font(text_layer, font);
  return text_layer;
}

GColor color_helper(GColor color, uint8_t inverted) {
  #ifdef PBL_BW
    if(inverted) {
      if(gcolor_equal(color, GColorBlack)) {
        color = GColorWhite;
      }
      else {
        color = GColorBlack;
      }
    }
  #else
    if(inverted) {
      color = color_inverted(color);
    }
  #endif
  return color;
}
