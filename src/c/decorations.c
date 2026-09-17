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
static TextLayer *weather_text_layer, *button_back_layer, *button_next_layer, *button_prev_layer;
static Layer *weather_icon_layer;
static BitmapLayer *logo_layer;
static GBitmap *logo_image;
static int8_t temp_now = INT8_MIN, temp_hi = INT8_MIN, temp_lo = INT8_MIN;
static WeatherCond weather_cond = WEATHER_COND_NONE;
static bool weather_painted = false;

static char weather_buf[32];

void weather_icon_layer_update_callback(Layer *my_layer, GContext* ctx);

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

/*
 * Push the user's button wording onto the three label layers. Empty labels are
 * legitimate: the arrow icon stays and the text disappears.
 */
static void decorations_apply_labels(void) {
  if (button_back_layer == NULL) {
    return;
  }
  text_layer_set_text(button_back_layer, global_settings.LabelBack);
  text_layer_set_text(button_prev_layer, global_settings.LabelPrev);
  text_layer_set_text(button_next_layer, global_settings.LabelNext);
}

void decorations_settings_callback() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "decorations_settings_callback()");

  text_layer_set_text_color(weather_text_layer, color_helper(colors[c_t2], global_settings.Invert));
  layer_mark_dirty(weather_icon_layer);

  text_layer_set_text_color(button_back_layer, color_helper(colors[c_d7], global_settings.Invert));
  text_layer_set_text_color(button_next_layer, color_helper(colors[c_d7], global_settings.Invert));
  text_layer_set_text_color(button_prev_layer, color_helper(colors[c_d7], global_settings.Invert));
  decorations_apply_labels();

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

  arrow_left_path_ptr = gpath_create(&ArrowLeftPathInfo);
  arrow_right_path_ptr = gpath_create(&ArrowRightPathInfo);

  wr_outer_path_ptr = gpath_create(&WaterResistOuterPathInfo);

  wr_outer_layer = layer_create(DECORATIONS_WR_OUTER);
  //layer_set_hidden(wr_outer_layer, true);
  layer_set_update_proc(wr_outer_layer, decorations_wr_outer_layer_update_callback);
  layer_add_child(decorations_layer, wr_outer_layer);

  // "CM" label removed per feedback — the WR box stays decorative for now.

  // WEATHER: conditions icon then "now°(high°/low°)", on the seconds' row
  // inside the panel. The corner readouts (was "WATER"/"RESIST") are gone: one
  // row reads better than two corners, and being on the panel's white fill the
  // readout takes an ink colour rather than the outside chrome's.
  weather_icon_layer = layer_create(DECORATIONS_WEATHER_ICON);
  layer_set_update_proc(weather_icon_layer, weather_icon_layer_update_callback);
  layer_add_child(decorations_layer, weather_icon_layer);

  weather_text_layer = text_layer_create_detailed(DECORATIONS_WEATHER_TEXT,
                                GColorClear, color_helper(colors[c_t2], global_settings.Invert),
                                GTextAlignmentLeft, font_tiny);
  layer_add_child(decorations_layer, text_layer_get_layer(weather_text_layer));

  decorations_update_weather();

  // BUTTON LABELS - text comes from the settings (capped and filtered there)
  button_back_layer = text_layer_create_detailed(DECORATIONS_BUTTON_BACK_LABEL, GColorClear
                                                 , color_helper(colors[c_d7], global_settings.Invert),
                                                 GTextAlignmentLeft, font_tiny);
  layer_add_child(decorations_layer, text_layer_get_layer(button_back_layer));

  button_next_layer = text_layer_create_detailed(DECORATIONS_BUTTON_NEXT_LABEL, GColorClear
                                                 , color_helper(colors[c_d7], global_settings.Invert),
                                                 GTextAlignmentRight, font_tiny);
  layer_add_child(decorations_layer, text_layer_get_layer(button_next_layer));

  button_prev_layer = text_layer_create_detailed(DECORATIONS_BUTTON_PREV_LABEL, GColorClear,
                                                  color_helper(colors[c_d7], global_settings.Invert),
                                                  GTextAlignmentRight, font_tiny);
  layer_add_child(decorations_layer, text_layer_get_layer(button_prev_layer));

  decorations_apply_labels();

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

  layer_remove_from_parent(text_layer_get_layer(weather_text_layer));
  layer_remove_from_parent(weather_icon_layer);
  layer_remove_from_parent(text_layer_get_layer(button_back_layer));
  layer_remove_from_parent(text_layer_get_layer(button_next_layer));
  layer_remove_from_parent(text_layer_get_layer(button_prev_layer));

  layer_remove_from_parent(button_back_icon_layer);
  layer_remove_from_parent(button_next_icon_layer);
  layer_remove_from_parent(button_prev_icon_layer);
  layer_remove_from_parent(wr_outer_layer);
  layer_remove_from_parent(decorations_layer);

  text_layer_destroy(weather_text_layer);
  layer_destroy(weather_icon_layer);
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
 * Weather values, as the phone sends them: current temperature, the day's high
 * and low, and a WMO condition code. INT8_MIN means "not known yet" for the
 * temperatures; each arriving key updates the row.
 */
void decorations_set_weather(int8_t now, int8_t hi, int8_t lo, uint8_t cond) {
  APP_LOG(APP_LOG_LEVEL_INFO, "decorations: weather now=%d hi=%d lo=%d cond=%d",
          (int)now, (int)hi, (int)lo, (int)cond);
  temp_now = now;
  temp_hi = hi;
  temp_lo = lo;
  weather_cond = weather_cond_from_wmo(cond);
  if (weather_icon_layer != NULL) {
    layer_mark_dirty(weather_icon_layer);
  }
  decorations_update_weather();
}

/*
 * One string on the seconds' row: "80°(88°/65°)" - current, then the high and
 * low. Parts the phone has not sent are left out rather than shown as dashes,
 * so half of an arriving message cannot read as real weather:
 *   all three -> 80°(88°/65°)    now only -> 80°    high/low only -> (88°/65°)
 *   nothing yet -> --
 *
 * The degree sign must be valid UTF-8 (0xC2 0xB0): a bare \xB0 is an invalid
 * sequence and Pebble's text renderer drops the whole string.
 */
/*
 * Condition glyph, 16x16, drawn from the same two colours as the readout: a
 * sun, a cloud, the cloud with rain under it, or the cloud with snow under it.
 * Primitives rather than GPaths: the shapes are circles and short strokes, and
 * at this size that is the whole drawing.
 */
void weather_icon_layer_update_callback(Layer *my_layer, GContext* ctx) {
  GColor color = color_helper(colors[c_t2], global_settings.Invert);
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_color(ctx, color);

  switch (weather_cond) {
    case WEATHER_COND_CLEAR:
      graphics_fill_circle(ctx, GPoint(7, 7), 3);
      graphics_draw_line(ctx, GPoint(7, 1), GPoint(7, 3));
      graphics_draw_line(ctx, GPoint(7, 11), GPoint(7, 13));
      graphics_draw_line(ctx, GPoint(1, 7), GPoint(3, 7));
      graphics_draw_line(ctx, GPoint(11, 7), GPoint(13, 7));
      graphics_draw_line(ctx, GPoint(3, 3), GPoint(4, 4));
      graphics_draw_line(ctx, GPoint(10, 10), GPoint(11, 11));
      graphics_draw_line(ctx, GPoint(11, 3), GPoint(10, 4));
      graphics_draw_line(ctx, GPoint(4, 10), GPoint(3, 11));
      break;

    case WEATHER_COND_CLOUD:
      graphics_fill_circle(ctx, GPoint(4, 8), 3);
      graphics_fill_circle(ctx, GPoint(8, 6), 4);
      graphics_fill_circle(ctx, GPoint(12, 8), 3);
      graphics_fill_rect(ctx, GRect(4, 6, 9, 5), 0, GCornerNone);
      break;

    case WEATHER_COND_RAIN:
      graphics_fill_circle(ctx, GPoint(4, 6), 3);
      graphics_fill_circle(ctx, GPoint(8, 4), 4);
      graphics_fill_circle(ctx, GPoint(12, 6), 3);
      graphics_fill_rect(ctx, GRect(4, 4, 9, 5), 0, GCornerNone);
      graphics_draw_line(ctx, GPoint(5, 11), GPoint(4, 14));
      graphics_draw_line(ctx, GPoint(9, 11), GPoint(8, 14));
      graphics_draw_line(ctx, GPoint(13, 11), GPoint(12, 14));
      break;

    case WEATHER_COND_SNOW:
      graphics_fill_circle(ctx, GPoint(4, 6), 3);
      graphics_fill_circle(ctx, GPoint(8, 4), 4);
      graphics_fill_circle(ctx, GPoint(12, 6), 3);
      graphics_fill_rect(ctx, GRect(4, 4, 9, 5), 0, GCornerNone);
      graphics_fill_circle(ctx, GPoint(5, 12), 1);
      graphics_fill_circle(ctx, GPoint(8, 14), 1);
      graphics_fill_circle(ctx, GPoint(11, 12), 1);
      break;

    case WEATHER_COND_NONE:
    default:
      break;  // no data yet: leave the space empty
  }
}

void decorations_update_weather() {
  if (weather_text_layer == NULL) {
    return;
  }

  bool have_now = temp_now != INT8_MIN;
  bool have_hl = temp_hi != INT8_MIN && temp_lo != INT8_MIN;

  if (have_now && have_hl) {
    snprintf(weather_buf, sizeof(weather_buf), "%d\xC2\xB0(%d\xC2\xB0/%d\xC2\xB0)",
             temp_now, temp_hi, temp_lo);
  }
  else if (have_now) {
    snprintf(weather_buf, sizeof(weather_buf), "%d\xC2\xB0", temp_now);
  }
  else if (have_hl) {
    snprintf(weather_buf, sizeof(weather_buf), "(%d\xC2\xB0/%d\xC2\xB0)", temp_hi, temp_lo);
  }
  else if (!weather_painted) {
    snprintf(weather_buf, sizeof(weather_buf), "--");
  }
  else {
    return;  // nothing to change; leave the last reading up
  }

  text_layer_set_text(weather_text_layer, weather_buf);
  weather_painted = true;
}
