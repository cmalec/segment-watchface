#include <pebble.h>
#include "_globals.h"
#include "bluetooth.h"
#include "vector.h"
#include "settings.h"
#include "helpers.h"
#include "animation.h"
#include "window.h"
#include "battery.h"
#include "health.h"

static Layer *bluetooth_layer, *bluetooth_icon_layer, *bluetooth_circle_layer;

static bool IsBluetoothConnected = true;

static GPath *bt_path_ptr = NULL;

static void bluetooth_badge_sync(void);

/*
bl1 Bluetooth Circle Connected
bl2 Bluetooth Icon Connected
bl3 Bluetooth Circle Disconnected
bl4 Bluetooth Icon Disconnected
*/
void bluetooth_settings_callback() {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "bluetooth_settings_callback()");
  // A settings change must not wait for a BT connect/disconnect event.
  bluetooth_badge_sync();
}

void bluetooth_circle_layer_update_callback(Layer *my_layer, GContext* ctx) {
  GColor color = color_helper(colors[c_bl1], global_settings.Invert);
  if(!IsBluetoothConnected) {
    color = color_helper(colors[c_bl3], global_settings.Invert);
  }
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_circle(ctx, GPoint(7, 7), 7);
}

void bluetooth_layer_update_callback(Layer *my_layer, GContext* ctx) {
  GColor color = color_helper(colors[c_bl2], global_settings.Invert);
  if(!IsBluetoothConnected) {
    color = color_helper(colors[c_bl4], global_settings.Invert);
  }
  graphics_context_set_stroke_color(ctx, color);
  gpath_draw_outline(ctx, bt_path_ptr);
}

/*
 * Show/hide the badge and re-anchor the top strip in one place: which badge
 * the user gets and how much room the strip reserves for it are the same
 * decision. The badge sits in the battery cluster on the right (badge, then
 * percentage, then icon), so it no longer takes the panel's left corner from
 * the health column.
 *
 * The slot is reserved for as long as the setting is on, not for as long as
 * the badge happens to be drawn: a phone going out of range must not shuffle
 * the battery readout across the strip.
 *
 * The vibe lives in bluetooth_connection_callback(), on the actual
 * connected->disconnected transition. Firing from here would re-pulse on
 * every unrelated call while disconnected (settings pushes, hourly
 * weather/phone-battery refreshes, tap/color switches), which is the "random
 * vibes" on a flaky link.
 */
static void bluetooth_badge_sync(void) {
  bool show = global_settings.BluetoothShow && IsBluetoothConnected;
  layer_set_hidden(bluetooth_circle_layer, !show);
  layer_set_hidden(bluetooth_icon_layer, !show);
  layer_mark_dirty(bluetooth_circle_layer);
  layer_mark_dirty(bluetooth_icon_layer);

  // battery.c owns the strip's geometry: it hands back the badge's slot and
  // re-bounds the health row behind it.
  layer_set_frame(bluetooth_layer, battery_cluster_slot(global_settings.BluetoothShow));
}

void bluetooth_connection_callback(bool connected) {
  bool dropped = !connected && IsBluetoothConnected;
  IsBluetoothConnected = connected;
  bluetooth_badge_sync();
  // One long pulse per real drop, never on reconnect or on unrelated calls.
  if (dropped && appStarted && global_settings.BluetoothVibe && !powerSaveEngaged) {
    vibes_long_pulse();
  }
}

void bluetooth_init() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "bluetooth_init()");

  bluetooth_layer = layer_create(BLUETOOTH_LAYER);
  //layer_set_hidden(bluetooth_layer, true);
  layer_add_child(my_window_layer, bluetooth_layer);
  
  bluetooth_circle_layer = layer_create(BLUETOOTH_ICON_CIRCLE);
  layer_set_update_proc(bluetooth_circle_layer, bluetooth_circle_layer_update_callback);
  layer_add_child(bluetooth_layer, bluetooth_circle_layer);
  
  bt_path_ptr = gpath_create(&BluetoothPathInfo);
  bluetooth_icon_layer = layer_create(BLUETOOTH_ICON_SYMBOL);
  layer_set_update_proc(bluetooth_icon_layer, bluetooth_layer_update_callback);
  layer_add_child(bluetooth_layer, bluetooth_icon_layer);

  bluetooth_connection_service_subscribe(bluetooth_connection_callback);

  // Seed state from peek WITHOUT routing through the callback: on launch
  // (appStarted=false) the vibe is suppressed anyway, but on power-save
  // re-init a stale IsBluetoothConnected=true + peek=false would mimic a
  // fresh drop and pulse for a disconnect that happened while unsubscribed.
  IsBluetoothConnected = bluetooth_connection_service_peek();
  bluetooth_badge_sync();

  settings_register_callback(bluetooth_settings_callback, SETTINGS_CALLBACK_BLUETOOTH);
  // Launch cascade (first init only).
  if (!appStarted) {
    animation_slide_in(bluetooth_layer, 650, DOWN);
  }
}

void bluetooth_deinit() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "bluetooth_deinit()");


  settings_unregister_callback(SETTINGS_CALLBACK_BLUETOOTH);
  bluetooth_connection_service_unsubscribe();
  gpath_destroy(bt_path_ptr);
  bt_path_ptr = NULL;
  
  layer_remove_from_parent(bluetooth_circle_layer);
  layer_remove_from_parent(bluetooth_icon_layer);
  layer_remove_from_parent(bluetooth_layer);
  
  layer_destroy(bluetooth_circle_layer);
  layer_destroy(bluetooth_icon_layer);
  layer_destroy(bluetooth_layer);
}
