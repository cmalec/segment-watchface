#include <pebble.h>
#include "fonts.h"

GFont font_big, font_small, font_tiny, font_weather, font_vollazee;

// Emery typography is authored directly for the 200x228 composition. The clock
// is the same size in both modes; only the seconds row is smaller, so no font
// is swapped at runtime and there is nothing for a settings callback to do.
#define FONT_BIG_SIZE      RESOURCE_ID_FONT_DIGITALE_99
#define FONT_SMALL_SIZE    RESOURCE_ID_FONT_DIGITALE_26
#define FONT_TINY_SIZE     RESOURCE_ID_FONT_LUCIDIA_14
// Weather-icons glyphs (SIL OFL 1.1, see README). The resource name carries the
// pixel height; the regex in package.json bakes only the conditions the face
// draws, so the 222-glyph webfont costs the watch a few hundred bytes.
#define FONT_WEATHER_SIZE  RESOURCE_ID_WX_ICON_16
// The bottom strip's three section labels ("Chrono" / "V1" / "Graph"), from
// resources/fonts/vollazee-font/Vollazee-2vx18.ttf. The regex in package.json
// bakes those ten glyphs only; the face never draws other text in it.
#define FONT_VOLLAZEE_SIZE RESOURCE_ID_FONT_VOLLAZEE_16

void fonts_init() {
  font_big = fonts_load_custom_font(resource_get_handle(FONT_BIG_SIZE));
  font_small = fonts_load_custom_font(resource_get_handle(FONT_SMALL_SIZE));
  font_tiny = fonts_load_custom_font(resource_get_handle(FONT_TINY_SIZE));
  font_weather = fonts_load_custom_font(resource_get_handle(FONT_WEATHER_SIZE));
  font_vollazee = fonts_load_custom_font(resource_get_handle(FONT_VOLLAZEE_SIZE));
}

void fonts_deinit() {
  fonts_unload_custom_font(font_big);
  fonts_unload_custom_font(font_small);
  fonts_unload_custom_font(font_tiny);
  fonts_unload_custom_font(font_weather);
  fonts_unload_custom_font(font_vollazee);
}
