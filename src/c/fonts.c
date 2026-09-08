#include <pebble.h>
#include "fonts.h"
#include "settings.h"

GFont font_big, font_small, font_tiny;

// Emery typography is authored directly for the 200x228 composition.
#define FONT_BIG_SIZE      RESOURCE_ID_FONT_DIGITALE_99
#define FONT_BIG_SEC_SIZE  RESOURCE_ID_FONT_DIGITALE_78
#define FONT_SMALL_SIZE    RESOURCE_ID_FONT_DIGITALE_31
#define FONT_TINY_SIZE     RESOURCE_ID_FONT_LUCIDIA_14

void fonts_settings_callback() {
  fonts_unload_custom_font(font_big);
  int32_t font_res = (global_settings.Seconds && !powerSaveEngaged)
      ? FONT_BIG_SEC_SIZE : FONT_BIG_SIZE;
  font_big = fonts_load_custom_font(resource_get_handle(font_res));
}

void fonts_init() {
  int32_t font_res = (global_settings.Seconds && !powerSaveEngaged)
      ? FONT_BIG_SEC_SIZE : FONT_BIG_SIZE;
  font_big = fonts_load_custom_font(resource_get_handle(font_res));
  font_small = fonts_load_custom_font(resource_get_handle(FONT_SMALL_SIZE));
  font_tiny = fonts_load_custom_font(resource_get_handle(FONT_TINY_SIZE));
  settings_register_callback(fonts_settings_callback, SETTINGS_CALLBACK_FONT);
}

void fonts_deinit() {
  fonts_unload_custom_font(font_big);
  fonts_unload_custom_font(font_small);
  fonts_unload_custom_font(font_tiny);
}
