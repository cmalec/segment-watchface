#include <pebble.h>
#include "_globals.h"
#include "battery.h"
#include "vector.h"
#include "settings.h"
#include "helpers.h"
#include "animation.h"
#include "window.h"
#include "fonts.h"
#include "health.h"


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

// Width the top strip's left column has to keep clear of the right-hand
// cluster. Set by battery_cluster_slot(); health.c bounds the step count with
// it.
static int16_t cluster_reserve = 0;

// Whether the badge's slot is part of the cluster, pushed in by bluetooth.c
// (the badge's owner): the user's icon setting, not the current connection
// state. Cached so battery_settings_callback, which doesn't own the badge, can
// re-run the same layout.
static bool cluster_badge_slot = false;

/*
 * Lay out the top strip's right-hand cluster for the current visibility
 * combination:
 *
 *   [bluetooth badge] gap [battery %] gap [battery icon] gap | panel outline
 *
 * Everything is derived right-to-left from the inner panel's edge, so the
 * reserve handed to health.c is always the width something is actually drawn
 * in. (The old magic numbers - 85 for percent+icon, 52 for percent-only - were
 * measured from the screen edge and ignored the badge entirely, which is how
 * the step count ended up painting over the heart rate.)
 *
 * Returns the bluetooth badge's slot so bluetooth.c can move its own layer;
 * the battery percentage is positioned here. Callers that don't own the badge
 * ignore the result.
 */
GRect battery_cluster_slot(bool bluetooth_slot_used) {
  bool show_percent = !global_settings.BatteryHide;
  bool show_icon = show_percent && !global_settings.BatteryIconOnly;
  cluster_badge_slot = bluetooth_slot_used;

  // Right edge (exclusive) the cluster's ink stops at.
  int16_t ink_right = PANEL_INNER_RIGHT - TOP_STRIP_EDGE_GAP;
  // Left edge of the left-most ink drawn so far, walking right-to-left. With
  // nothing drawn yet there is nothing to keep clear of, so the first member
  // takes the cluster's right edge.
  int16_t ink_left = ink_right;

  if (show_icon) {
    ink_left = BATTERY_LAYER.origin.x;
  }

  if (show_percent && battery_percent_layer != NULL) {
    // Right-aligned text: only the box's right edge places it.
    GRect pct = BATTERY_PERCENT;
    pct.origin.x = (show_icon ? BATTERY_LAYER.origin.x - TOP_STRIP_ITEM_GAP : ink_right)
                   - BATTERY_PERCENT_W;
    layer_set_frame(text_layer_get_layer(battery_percent_layer), pct);
    ink_left = pct.origin.x + BATTERY_PERCENT_W - BATTERY_PERCENT_INK_W;
  }

  GRect badge = BLUETOOTH_LAYER;
  if (bluetooth_slot_used) {
    bool drawn = show_icon || show_percent;
    badge.origin.x = (drawn ? ink_left - TOP_STRIP_ITEM_GAP : ink_right)
                     - BLUETOOTH_BADGE_INK_W;
    ink_left = badge.origin.x;
  }

  cluster_reserve = PANEL_INNER_RIGHT - ink_left;
  // The health row is bounded by this reserve. Re-bound it here so every
  // caller of the strip layout gets the pair in step for free.
  health_layout_row();
  return badge;
}

int16_t battery_top_reserve(void) {
  return cluster_reserve;
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
  // Percent-only / hidden change what the cluster occupies, which changes both
  // the percentage's own anchor and the reserve the health row keeps clear.
  battery_cluster_slot(cluster_badge_slot);
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
    const int16_t inner_max_w = 17;
    int16_t width = (batteryPercent * inner_max_w) / 100 + 1;
    if (width > inner_max_w) {
      width = inner_max_w;
    }
    graphics_fill_rect(ctx, GRect(3, 3, width, 7), 0, GCornerNone);
  }
}


void battery_init() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_init()");

  battery_percent_layer = text_layer_create_detailed(BATTERY_PERCENT,
                                GColorClear, color_helper(colors[c_bi1], global_settings.Invert),
                                GTextAlignmentRight, font_tiny);
  layer_add_child(my_window_layer, text_layer_get_layer(battery_percent_layer));

  bolt_path_ptr = gpath_create(&BatteryBoltPathInfo);
  battery_layer = layer_create(BATTERY_LAYER);
  layer_set_update_proc(battery_layer, battery_layer_update_callback);
  layer_add_child(my_window_layer, battery_layer);

  battery_update(battery_state_service_peek());
  battery_state_service_subscribe(&battery_update);

  settings_register_callback(battery_settings_callback, SETTINGS_CALLBACK_BATTERY);

  // Phone battery bar: starts hidden until the phone reports a level.
  phone_batt_layer = layer_create(PHONE_BATT_BAR);
  layer_set_update_proc(phone_batt_layer, phone_batt_layer_update_callback);
  layer_set_hidden(phone_batt_layer, true);
  layer_add_child(my_window_layer, phone_batt_layer);

  // Apply the persisted hide settings immediately (the settings callback only
  // fires on an incoming settings message, which may never come). The cluster
  // starts without the badge; bluetooth_init() pushes the real visibility.
  battery_apply_visibility();
  battery_cluster_slot(false);

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
  battery_percent_layer = NULL;

  layer_remove_from_parent(phone_batt_layer);
  layer_destroy(phone_batt_layer);
  phone_batt_layer = NULL;

  layer_remove_from_parent(battery_layer);

  layer_destroy(battery_layer);
  battery_layer = NULL;

  // Nothing of the cluster is drawn any more (power save tears the battery and
  // the badge down together), so release the space for the health column.
  cluster_reserve = 0;
  health_layout_row();
}
