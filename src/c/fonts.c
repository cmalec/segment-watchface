#include <pebble.h>
#include "fonts.h"
#include "settings.h"

GFont font_big, font_small, font_tiny;

// DS-Digital renders at ~0.507 px of glyph width per px of font size
// (measured: 36.14 px advance for '0' at 71 px). Emery is a 1.389x basalt
// display, so emery font sizes are the basalt ones scaled by 1.389 and the
// original digit box layout carries over unchanged.
#ifdef PBL_PLATFORM_EMERY
#define FONT_BIG_SIZE   RESOURCE_ID_FONT_DIGITALE_99
#define FONT_BIG_SEC_SIZE RESOURCE_ID_FONT_DIGITALE_78
#define FONT_SMALL_SIZE RESOURCE_ID_FONT_DIGITALE_31
#define FONT_TINY_SIZE  RESOURCE_ID_FONT_LUCIDIA_14
#else
#define FONT_BIG_SIZE   RESOURCE_ID_FONT_DIGITALE_71
#define FONT_BIG_SEC_SIZE RESOURCE_ID_FONT_DIGITALE_56
#define FONT_SMALL_SIZE RESOURCE_ID_FONT_DIGITALE_22
#define FONT_TINY_SIZE  RESOURCE_ID_FONT_LUCIDIA_10
#endif

void fonts_settings_callback() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "fonts_settings_callback()");

  fonts_unload_custom_font(font_big);
  int32_t font_res= (global_settings.Seconds && !powerSaveEngaged) ? FONT_BIG_SEC_SIZE : FONT_BIG_SIZE;
  font_big = fonts_load_custom_font(resource_get_handle(font_res));
}
void fonts_init() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "fonts_init()");

  int32_t font_res= (global_settings.Seconds && !powerSaveEngaged) ? FONT_BIG_SEC_SIZE : FONT_BIG_SIZE;
  font_big = fonts_load_custom_font(resource_get_handle(font_res));
  font_small = fonts_load_custom_font(resource_get_handle(FONT_SMALL_SIZE));
  font_tiny = fonts_load_custom_font(resource_get_handle(FONT_TINY_SIZE));
  settings_register_callback(fonts_settings_callback, SETTINGS_CALLBACK_FONT);

}
void fonts_deinit() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "fonts_deinit()");
  
  fonts_unload_custom_font(font_big);
  fonts_unload_custom_font(font_small);
  fonts_unload_custom_font(font_tiny);
}
