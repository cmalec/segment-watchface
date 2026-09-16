#include <pebble.h>
#include "fonts.h"

GFont font_big, font_small, font_tiny;

// Emery typography is authored directly for the 200x228 composition. The clock
// is the same size in both modes; only the seconds row is smaller, so no font
// is swapped at runtime and there is nothing for a settings callback to do.
#define FONT_BIG_SIZE      RESOURCE_ID_FONT_DIGITALE_99
#define FONT_SMALL_SIZE    RESOURCE_ID_FONT_DIGITALE_26
#define FONT_TINY_SIZE     RESOURCE_ID_FONT_LUCIDIA_14

void fonts_init() {
  font_big = fonts_load_custom_font(resource_get_handle(FONT_BIG_SIZE));
  font_small = fonts_load_custom_font(resource_get_handle(FONT_SMALL_SIZE));
  font_tiny = fonts_load_custom_font(resource_get_handle(FONT_TINY_SIZE));
}

void fonts_deinit() {
  fonts_unload_custom_font(font_big);
  fonts_unload_custom_font(font_small);
  fonts_unload_custom_font(font_tiny);
}
