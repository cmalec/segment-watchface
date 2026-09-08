#include <pebble.h>
#include "decorations.h"
#include "_globals.h"
#include "vector.h"
#include "helpers.h"
#include "animation.h"
#include "settings.h"
#include "fonts.h"
#include "window.h"

static Layer *decorations_layer, *wr_outer_layer, *button_back_icon_layer, *button_next_icon_layer, *button_prev_icon_layer;
static TextLayer *temp_hi_layer, *temp_lo_layer, *button_back_layer, *button_next_layer, *button_prev_layer;
static BitmapLayer *logo_layer;
static GBitmap *logo_image;
static int8_t temp_hi = INT8_MIN, temp_lo = INT8_MIN;
static int8_t last_displayed_hi = INT8_MIN, last_displayed_lo = INT8_MIN;

static char temp_hi_buf[8], temp_lo_buf[8];

static GPath *arrow_left_path_ptr = NULL;
static GPath *arrow_right_path_ptr = NULL;
static GPath *wr_outer_path_ptr = NULL;

/*
d1  Horizontal Line Top
d2  Horizontal Line Bottom
d3  Water Resist Box Stroke
d4  Water Resist Box Fill
d5  WR Letters
d6  Water Resist Text
d7  Button Labels
d8  Button Icons
d9  Branding Text
*/

void decorations_settings_callback() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "decorations_settings_callback()");

  text_layer_set_text_color(temp_hi_layer, color_helper(colors[c_d6], global_settings.Invert));
  text_layer_set_text_color(temp_lo_layer, color_helper(colors[c_d6], global_settings.Invert));

  text_layer_set_text_color(button_back_layer, color_helper(colors[c_d7], global_settings.Invert));
  text_layer_set_text_color(button_next_layer, color_helper(colors[c_d7], global_settings.Invert));
  text_layer_set_text_color(button_prev_layer, color_helper(colors[c_d7], global_settings.Invert));

  #ifdef PBL_COLOR
      GColor * xcolors = gbitmap_get_palette(logo_image);
      xcolors[0].argb = color_helper(colors[c_bg4], global_settings.Invert).argb;
      xcolors[1].argb = color_helper(colors[c_d9], global_settings.Invert).argb;
  #endif

  if(global_settings.BrandingMask) {
    layer_set_hidden(bitmap_layer_get_layer(logo_layer), true);
  }
  else {
    layer_set_hidden(bitmap_layer_get_layer(logo_layer), false);
  }

  layer_mark_dirty(decorations_layer);
}

void back_icon_layer_update_callback(Layer *my_layer, GContext* ctx) {
  //Arrow BACK
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_d8], global_settings.Invert));
  graphics_context_set_fill_color(ctx, color_helper(colors[c_d8], global_settings.Invert));
  gpath_draw_filled(ctx, arrow_left_path_ptr);
  gpath_draw_outline(ctx, arrow_left_path_ptr);
}


void next_icon_layer_update_callback(Layer *my_layer, GContext* ctx) {
  //Arrow NEXT
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_d8], global_settings.Invert));
  graphics_context_set_fill_color(ctx, color_helper(colors[c_d8], global_settings.Invert));
  gpath_draw_filled(ctx, arrow_right_path_ptr);
  gpath_draw_outline(ctx, arrow_right_path_ptr);
}


void prev_icon_layer_update_callback(Layer *my_layer, GContext* ctx) {
  //Arrow PREV
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_d8], global_settings.Invert));
  graphics_context_set_fill_color(ctx, color_helper(colors[c_d8], global_settings.Invert));
  gpath_draw_filled(ctx, arrow_right_path_ptr);
  gpath_draw_outline(ctx, arrow_right_path_ptr);
}

void decorations_layer_update_callback(Layer *my_layer, GContext* ctx) {
  //Line Top
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_d1], global_settings.Invert));
  graphics_draw_line(ctx, DECORATIONS_LINE_TOP_START, DECORATIONS_LINE_TOP_END);

  //Line Bottom
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_d2], global_settings.Invert));
  graphics_draw_line(ctx, DECORATIONS_LINE_BOTTOM_START, DECORATIONS_LINE_BOTTOM_END);
}

void decorations_wr_outer_layer_update_callback(Layer *my_layer, GContext* ctx) {

  //Box surrounding WR
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_d3], global_settings.Invert));
  graphics_context_set_fill_color(ctx, color_helper(colors[c_d4], global_settings.Invert));
  gpath_draw_filled(ctx, wr_outer_path_ptr);
  gpath_draw_outline(ctx, wr_outer_path_ptr);

}

void decorations_init() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "decorations_init()");

  decorations_layer = layer_create(FULLSCREEN);
  //layer_set_hidden(decorations_layer, true);
  layer_add_child(my_window_layer, decorations_layer);
  layer_set_update_proc(decorations_layer, decorations_layer_update_callback);


  button_back_icon_layer = layer_create(DECORATIONS_BUTTON_BACK_ICON);
  layer_set_update_proc(button_back_icon_layer, back_icon_layer_update_callback);
  layer_add_child(decorations_layer, button_back_icon_layer);

  button_next_icon_layer = layer_create(DECORATIONS_BUTTON_NEXT_ICON);
  layer_set_update_proc(button_next_icon_layer, next_icon_layer_update_callback);
  layer_add_child(decorations_layer, button_next_icon_layer);

  button_prev_icon_layer = layer_create(DECORATIONS_BUTTON_PREV_ICON);
  layer_set_update_proc(button_prev_icon_layer, prev_icon_layer_update_callback);
  layer_add_child(decorations_layer, button_prev_icon_layer);

  arrow_left_path_ptr = vector_create(&ArrowLeftPathInfo);
  arrow_right_path_ptr = vector_create(&ArrowRightPathInfo);

  wr_outer_path_ptr = vector_create(&WaterResistOuterPathInfo);

  wr_outer_layer = layer_create(DECORATIONS_WR_OUTER);
  //layer_set_hidden(wr_outer_layer, true);
  layer_set_update_proc(wr_outer_layer, decorations_wr_outer_layer_update_callback);
  layer_add_child(decorations_layer, wr_outer_layer);

  // "CM" label removed per feedback — the WR box stays decorative for now.

  // DAY HIGH TEMP (was "WATER")
  temp_hi_layer = text_layer_create_detailed(DECORATIONS_TEMP_HI, false,
                                GColorClear, color_helper(colors[c_d6], global_settings.Invert),
                                GTextAlignmentRight, font_tiny);
  layer_add_child(decorations_layer, text_layer_get_layer(temp_hi_layer));

  // DAY LOW TEMP (was "RESIST")
  temp_lo_layer = text_layer_create_detailed(DECORATIONS_TEMP_LO, false,
                                GColorClear, color_helper(colors[c_d6], global_settings.Invert),
                                GTextAlignmentLeft, font_tiny);
  layer_add_child(decorations_layer, text_layer_get_layer(temp_lo_layer));

  // Show "--" placeholders now that both layers exist (update_temp_layers
  // was previously called before temp_lo_layer existed, so the placeholder
  // never rendered and temps only appeared once data arrived).
  decorations_update_temp_layers();

  // BACK BUTTON LABEL
  button_back_layer = text_layer_create_detailed(DECORATIONS_BUTTON_BACK_LABEL, false, GColorClear
                                                 , color_helper(colors[c_d7], global_settings.Invert),
                                                 GTextAlignmentLeft, font_tiny);
  text_layer_set_text(button_back_layer, "LIGHT");
  layer_add_child(decorations_layer, text_layer_get_layer(button_back_layer));


  // NEXT BUTTON LABEL
  button_next_layer = text_layer_create_detailed(DECORATIONS_BUTTON_NEXT_LABEL, false, GColorClear
                                                 , color_helper(colors[c_d7], global_settings.Invert),
                                                 GTextAlignmentRight, font_tiny);
  text_layer_set_text(button_next_layer, "NEXT");
  layer_add_child(decorations_layer, text_layer_get_layer(button_next_layer));


  // PREV BUTTON LABEL
  button_prev_layer = text_layer_create_detailed(DECORATIONS_BUTTON_PREV_LABEL, false, GColorClear,
                                                  color_helper(colors[c_d7], global_settings.Invert),
                                                  GTextAlignmentRight, font_tiny);
  text_layer_set_text(button_prev_layer, "PREV");
  layer_add_child(decorations_layer, text_layer_get_layer(button_prev_layer));

  // BRANDING LABEL
  logo_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_PEBBLE);
  logo_layer = bitmap_layer_create(DECORATIONS_LOGO);
  #ifdef PBL_COLOR
    GColor * xcolors = gbitmap_get_palette(logo_image);
    xcolors[0].argb = color_helper(colors[c_bg4], global_settings.Invert).argb;
    xcolors[1].argb = color_helper(colors[c_d9], global_settings.Invert).argb;
  #endif
  bitmap_layer_set_bitmap(logo_layer, logo_image);
  layer_add_child(decorations_layer, bitmap_layer_get_layer(logo_layer));

  // WATER/TEMP + CM LABELS
  // (temp layers are set up further down)

  settings_register_callback(decorations_settings_callback, SETTINGS_CALLBACK_DECORATIONS);

  // Launch cascade (first init only).
  if (!appStarted) {
    animation_slide_in(decorations_layer, 450, RIGHT);
    animation_slide_in(wr_outer_layer, 700, UP);
  }

  // Emery is rectangular, so all decoration layers remain visible.

  //animation_slide_in(decorations_layer, 700, RIGHT);
  //animation_slide_in(wr_outer_layer, 700, UP);
}

void decorations_deinit() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "decorations_init()");

  gpath_destroy(wr_outer_path_ptr);
  gpath_destroy(arrow_left_path_ptr);
  gpath_destroy(arrow_right_path_ptr);

  wr_outer_path_ptr = NULL;
  arrow_left_path_ptr = NULL;
  arrow_right_path_ptr = NULL;

  layer_remove_from_parent(bitmap_layer_get_layer(logo_layer));
  gbitmap_destroy(logo_image);
  logo_image = NULL;
  bitmap_layer_destroy(logo_layer);

  layer_remove_from_parent(text_layer_get_layer(temp_hi_layer));
  layer_remove_from_parent(text_layer_get_layer(temp_lo_layer));
  layer_remove_from_parent(text_layer_get_layer(button_back_layer));
  layer_remove_from_parent(text_layer_get_layer(button_next_layer));
  layer_remove_from_parent(text_layer_get_layer(button_prev_layer));

  layer_remove_from_parent(button_back_icon_layer);
  layer_remove_from_parent(button_next_icon_layer);
  layer_remove_from_parent(button_prev_icon_layer);
  layer_remove_from_parent(wr_outer_layer);
  layer_remove_from_parent(decorations_layer);

  text_layer_destroy(temp_hi_layer);
  text_layer_destroy(temp_lo_layer);
  text_layer_destroy(button_back_layer);
  text_layer_destroy(button_next_layer);
  text_layer_destroy(button_prev_layer);

  layer_destroy(button_back_icon_layer);
  layer_destroy(button_next_icon_layer);
  layer_destroy(button_prev_icon_layer);
  layer_destroy(wr_outer_layer);
  layer_destroy(decorations_layer);
}

void decorations_toggle(bool is_obstructed) {
  if(is_obstructed) {
    layer_set_hidden(decorations_layer, true);
  } else {
    layer_set_hidden(decorations_layer, false);
  }
}

/*
 * Weather readouts: day high (left, was WATER) and day low (right, was
 * RESIST). Values arrive from the phone as Celsius ints (negative values
 * supported); temps display as e.g. "24°" / "12°".
 */
void decorations_set_temps(int8_t hi, int8_t lo) {
  APP_LOG(APP_LOG_LEVEL_INFO, "decorations: set temps hi=%d lo=%d", (int)hi, (int)lo);
  temp_hi = hi;
  temp_lo = lo;
  decorations_update_temp_layers();
}

void decorations_update_temp_layers() {
  if (temp_hi_layer == NULL || temp_lo_layer == NULL) {
    return;
  }

  if (temp_hi != INT8_MIN) {
    // NOTE: degree sign must be valid UTF-8 (0xC2 0xB0). A bare \xB0 is an
    // invalid sequence and Pebble's text renderer drops the WHOLE string.
    snprintf(temp_hi_buf, sizeof(temp_hi_buf), "%d\xC2\xB0", temp_hi);
    text_layer_set_text(temp_hi_layer, temp_hi_buf);
    last_displayed_hi = temp_hi;
  }
  else if (last_displayed_hi == INT8_MIN) {
    // No weather data yet: show a dash rather than stale placeholder text
    text_layer_set_text(temp_hi_layer, "--");
  }

  if (temp_lo != INT8_MIN) {
    snprintf(temp_lo_buf, sizeof(temp_lo_buf), "%d\xC2\xB0", temp_lo);
    text_layer_set_text(temp_lo_layer, temp_lo_buf);
    last_displayed_lo = temp_lo;
  }
  else if (last_displayed_lo == INT8_MIN) {
    text_layer_set_text(temp_lo_layer, "--");
  }
}
