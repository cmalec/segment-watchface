#include <pebble.h>
#include "_globals.h"
#include "battery.h"
#include "vector.h"
#include "settings.h"
#include "helpers.h"
#include "animation.h"
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

/*
bi1 Battery Indicator
bi2 Battery Indicator Warning
bi3 Battery Indicator Critical
bi4 Battery Charging
*/

// Width (px) reserved at the right edge of the top strip for the battery
// readout, used by health.c to position the steps/BPM row dynamically.
// Icon mode: icon + percent. Percent-only: percent alone. Respects
// BatteryHide (everything hidden -> no reserved space on that side... but
// health stays put; it simply has room to breathe).
int16_t battery_right_margin(void) {
  #ifdef PBL_PLATFORM_EMERY
    if (global_settings.BatteryHide) return 0;
    if (global_settings.BatteryIconOnly) return 52;   // percent text only
    return 85;                                        // percent + icon
  #else
    if (global_settings.BatteryHide) return 0;
    if (global_settings.BatteryIconOnly) return 38;
    return 60;
  #endif
}

// Left origin (screen x) for the health row. With the BT badge hidden, the
// row slides left to use the freed space.
int16_t health_left_origin(void) {
  #ifdef PBL_PLATFORM_EMERY
    return global_settings.BluetoothShow ? 25 : 10;
  #else
    return global_settings.BluetoothShow ? 25 : 12;
  #endif
}

void battery_apply_visibility() {
  bool hideAll = global_settings.BatteryHide;
  bool iconOnly = global_settings.BatteryIconOnly;
  layer_set_hidden(battery_layer, hideAll || iconOnly);
  layer_set_hidden(text_layer_get_layer(battery_percent_layer), hideAll);
  if (phone_batt_layer != NULL) {
    // Phone bar shows only when the icon is visible (percent-only mode
    // deliberately drops it).
    bool showBar = !hideAll && !iconOnly && phoneBattValid;
    layer_set_hidden(phone_batt_layer, !showBar);
  }
}

void battery_settings_callback() {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_settings_callback()");
  // Refresh text color via the charge-state path (handles warning/critical
  // states AND the invert flag uniformly).
  battery_update(battery_state_service_peek());
  battery_apply_visibility();
  layer_mark_dirty(text_layer_get_layer(battery_percent_layer));
  layer_mark_dirty(battery_layer);
  #if defined(PBL_PLATFORM_EMERY) && defined(PBL_HEALTH)
  // The battery reserve width may have changed (icon vs percent-only vs
  // hidden); re-anchor the heart-rate readout.
  extern void health_layout_row();
  if (global_settings.Health) {
    health_layout_row();
  }
  #endif
}

void battery_update(BatteryChargeState charge_state) {
  batteryPercent = charge_state.charge_percent;
  batteryCharging = charge_state.is_charging;
  if(batteryPercent==0) {
    batteryPercent=1;
  }
  layer_set_hidden(text_layer_get_layer(battery_percent_layer), global_settings.BatteryHide);
  static char txt[5];
  snprintf(txt, sizeof(txt), "%u%%", batteryPercent);
  text_layer_set_text(battery_percent_layer, txt);

  // Percent text color tracks the fill color (warning/critical states);
  // this lives here, NOT in the draw callback (which is for drawing only).
  GColor color = color_helper(colors[c_bi1], global_settings.Invert);
  if(batteryPercent<20) {
    color = color_helper(colors[c_bi3], global_settings.Invert);
  }
  else if(batteryPercent<30) {
    color = color_helper(colors[c_bi2], global_settings.Invert);
  }
  text_layer_set_text_color(battery_percent_layer, color);
  layer_mark_dirty(battery_layer);
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
  if (phone_batt_layer != NULL && !global_settings.BatteryHide && !global_settings.BatteryIconOnly) {
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

  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_rect(ctx, BATTERY_ICON);
  graphics_draw_rect(ctx, BATTERY_ICON_TERMINAL);

  graphics_context_set_fill_color(ctx, color);

  if(batteryCharging) {
    graphics_context_set_stroke_color(ctx, color_helper(colors[c_bi4], global_settings.Invert));
    gpath_draw_outline(ctx, bolt_path_ptr);
  }
  else {
    // Fill width proportional to charge, clamped to the icon's inner width
    // so 100% can't overflow the border by a pixel.
    #ifdef PBL_PLATFORM_EMERY
      const int16_t inner_max_w = 17;
    #else
      const int16_t inner_max_w = 11;
    #endif
    int16_t width = (batteryPercent * inner_max_w) / 100 + 1;
    if (width > inner_max_w) {
      width = inner_max_w;
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

  bolt_path_ptr = vector_create(&BatteryBoltPathInfo);
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

  // Apply the persisted hide settings immediately (the settings callback only
  // fires on an incoming settings message, which may never come).
  battery_apply_visibility();

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
