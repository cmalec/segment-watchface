#include <pebble.h>
#include "_globals.h"
#include "battery.h"
#include "settings.h"
#include "helpers.h"
#include "window.h"
#include "fonts.h"


static Layer *battery_layer;
static TextLayer *battery_percent_layer;

static Layer *phone_batt_layer;      // thin bar under the watch battery
static bool phoneBattValid = false;
static uint8_t phoneBattPercent = 0;

static bool batteryCharging = false;
static uint8_t batteryPercent;

static GPath *bolt_path_ptr = NULL;

static GPathInfo BOLT_PATH_INFO = {
  .num_points = 13,
  .points = (GPoint []) {{4,4},{6,4},{6,3},{8,3},{8,2},{8,4},{12,4},{10,4},{10,5},{8,5},{8,6},{8,4},{11,4}}
};

/*
bi1 Battery Indicator
bi2 Battery Indicator Warning
bi3 Battery Indicator Critical
bi4 Battery Charging
*/

void battery_settings_callback() {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_settings_callback()");
  text_layer_set_text_color(battery_percent_layer, color_helper(colors[c_bi1], global_settings.Invert));
  layer_set_hidden(battery_layer, global_settings.BatteryHide);
  layer_set_hidden(text_layer_get_layer(battery_percent_layer), global_settings.BatteryHide);
  layer_mark_dirty(text_layer_get_layer(battery_percent_layer));
  layer_mark_dirty(battery_layer);
}

void battery_update(BatteryChargeState charge_state) {
  batteryPercent = charge_state.charge_percent;
  batteryCharging = charge_state.is_charging;
  if(batteryPercent==0) {
    batteryPercent=1;
  }
  /*if(batteryPercent==100) {
    layer_set_hidden(text_layer_get_layer(battery_percent_layer),true);
  }
  else {*/
    layer_set_hidden(text_layer_get_layer(battery_percent_layer), global_settings.BatteryHide);
    static char txt[5];
    snprintf(txt, sizeof(txt), "%u%%", batteryPercent);
    text_layer_set_text(battery_percent_layer, txt);
  //}

}

// Phone battery bar: only drawn once the phone has actually reported a
// level, so devices where the JS Battery API is unavailable show nothing.
static void phone_batt_layer_update_callback(Layer *my_layer, GContext* ctx) {
  if (!phoneBattValid) {
    return;
  }
  GRect bounds = layer_get_bounds(my_layer);
  GColor color = color_helper(colors[c_bi1], global_settings.Invert);
  if (phoneBattPercent < 20) {
    color = color_helper(colors[c_bi3], global_settings.Invert);
  }
  graphics_context_set_fill_color(ctx, color);
  // 1px inset all round; fill width proportional (inner width = w - 2)
  int16_t inner_w = bounds.size.w - 2;
  int16_t fill_w = (inner_w * phoneBattPercent) / 100;
  if (fill_w < 1) {
    fill_w = 1;
  }
  graphics_fill_rect(ctx, GRect(1, 1, fill_w, bounds.size.h - 2), 0, GCornerNone);
}

void battery_set_phone_percent(uint8_t percent) {
  phoneBattPercent = percent;
  phoneBattValid = true;
  if (phone_batt_layer != NULL) {
    layer_set_hidden(phone_batt_layer, false);
    layer_mark_dirty(phone_batt_layer);
  }
}

void battery_layer_update_callback(Layer *my_layer, GContext* ctx) {
  GColor color = color_helper(colors[c_bi1], global_settings.Invert);
  if(batteryPercent<20) {
    color = color_helper(colors[c_bi3], global_settings.Invert);
  }
  else if(batteryPercent<30) {
    color = color_helper(colors[c_bi2], global_settings.Invert);
  }

  text_layer_set_text_color(battery_percent_layer, color);

  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_rect(ctx, BATTERY_ICON);
  graphics_draw_rect(ctx, BATTERY_ICON_TERMINAL);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, color);

  if(batteryCharging) {
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_bi4], global_settings.Invert));
  gpath_draw_outline(ctx, bolt_path_ptr);
  }
  else {
  // Inner width of the battery icon (border + terminal inset accounted).
  #ifdef PBL_PLATFORM_EMERY
    const float inner_max_w = 17.0f;
  #else
    const float inner_max_w = 11.0f;
  #endif
  uint8_t width = ((batteryPercent/100.0)*inner_max_w);
  if(width<inner_max_w+1) {
    width++;
  }
    #ifdef PBL_PLATFORM_EMERY
    graphics_fill_rect(ctx, GRect(3, 3, width, 7), 0, GCornerNone);
    #else
    graphics_fill_rect(ctx, GRect(2, 2, width, 5), 0, GCornerNone);
    #endif
  }
}


void battery_init() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_init()");

  battery_percent_layer = text_layer_create_detailed(BATTERY_PERCENT, false,
                                GColorClear, color_helper(colors[c_bi1], global_settings.Invert),
                                PBL_IF_RECT_ELSE(GTextAlignmentRight, GTextAlignmentLeft), font_tiny);
  layer_add_child(my_window_layer, text_layer_get_layer(battery_percent_layer));

  bolt_path_ptr = gpath_create(&BOLT_PATH_INFO);
  battery_layer = layer_create(BATTERY_LAYER);
  //layer_set_hidden(battery_layer, true);
  layer_set_update_proc(battery_layer, battery_layer_update_callback);
  #if defined(PBL_RECT)
    layer_add_child(my_window_layer, battery_layer);
  #endif

  battery_update(battery_state_service_peek());
  battery_state_service_subscribe(&battery_update);

  settings_register_callback(battery_settings_callback, SETTINGS_CALLBACK_BATTERY);

  // Phone battery bar: starts hidden until the phone reports a level.
  phone_batt_layer = layer_create(PHONE_BATT_BAR);
  layer_set_update_proc(phone_batt_layer, phone_batt_layer_update_callback);
  layer_set_hidden(phone_batt_layer, true);
  layer_add_child(my_window_layer, phone_batt_layer);

  // Apply the persisted hide setting immediately (the settings callback only
  // fires on an incoming settings message, which may never come).
  layer_set_hidden(battery_layer, global_settings.BatteryHide);
  layer_set_hidden(text_layer_get_layer(battery_percent_layer), global_settings.BatteryHide);

  // Launch cascade (first init only — not on settings re-init).
  if (!appStarted) {
    animation_slide_in(battery_layer, 550, DOWN);
    animation_slide_in(text_layer_get_layer(battery_percent_layer), 600, DOWN);
  }

  //animation_slide_in(battery_layer, 1000, DOWN);
  //animation_slide_in(text_layer_get_layer(battery_percent_layer), 1100, DOWN);
}

void battery_deinit() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_deinit()");

  settings_unregister_callback(SETTINGS_CALLBACK_BATTERY);
  battery_state_service_unsubscribe();
  gpath_destroy(bolt_path_ptr);
  bolt_path_ptr = NULL;

  text_layer_destroy(battery_percent_layer);

  layer_remove_from_parent(phone_batt_layer);
  layer_destroy(phone_batt_layer);
  phone_batt_layer = NULL;

  layer_remove_from_parent(battery_layer);

  layer_destroy(battery_layer);
}
