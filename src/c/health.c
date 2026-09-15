#include <pebble.h>
#include "_globals.h"
#include "health.h"
#include "vector.h"
#include "settings.h"
#include "helpers.h"
#include "animation.h"
#include "window.h"
#include "fonts.h"
#include "battery.h"

static bool health_enabled = false;

#ifdef PBL_HEALTH

static TextLayer *health_text_layer;
static Layer *health_layer, *health_foot_layer, *health_foot2_layer, *health_zee_layer;
static Layer *health_bpm_row_layer, *health_bpm_icon_layer;
static TextLayer *health_bpm_layer;

// GPaths are native Emery geometry defined in vector.c.

static GPath *foot_path_ptr = NULL;
static GPath *heel_path_ptr = NULL;
static GPath *zee1_path_ptr = NULL;
static GPath *zee2_path_ptr = NULL;
static GPath *zee3_path_ptr = NULL;

// Heart outline, used on emery (Pebble Time 2 has a HRM).
static GPath *heart_path_ptr = NULL;

static HealthValue s_sleep, s_steps;
static HealthValue s_bpm;

void health_icon_layer_update_callback(Layer *my_layer, GContext* ctx) {
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_t2], global_settings.Invert));
  graphics_context_set_fill_color(ctx, color_helper(colors[c_t2], global_settings.Invert));
  if(s_steps < HEALTH_STEP_MIN) {
    //zzz
    gpath_draw_outline_open(ctx, zee1_path_ptr);
    gpath_draw_outline_open(ctx, zee2_path_ptr);
    gpath_draw_outline_open(ctx, zee3_path_ptr);
  } else {
    //steps
    gpath_draw_filled(ctx, foot_path_ptr);
    gpath_draw_outline(ctx, foot_path_ptr);
    gpath_draw_filled(ctx, heel_path_ptr);
    gpath_draw_outline(ctx, heel_path_ptr);
  }
}

static void health_bpm_icon_layer_update_callback(Layer *my_layer, GContext* ctx) {
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_t2], global_settings.Invert));
  graphics_context_set_fill_color(ctx, color_helper(colors[c_t2], global_settings.Invert));
  gpath_draw_filled(ctx, heart_path_ptr);
  gpath_draw_outline(ctx, heart_path_ptr);
}

void health_settings_callback() {
  health_deinit();
  health_init();
}

void health_update() {
  static char str[20], str2[20];
  if(s_steps < HEALTH_STEP_MIN) {
    //zzz
    int hours = 0, minutes = 0;
    duration_to_time(s_sleep, &hours, &minutes);

    if(hours>0) {
      snprintf(str2, sizeof(str2), "%dH'%dM", hours, minutes);
    } else {
      snprintf(str2, sizeof(str2), "%dM", minutes);
    }

    layer_set_hidden(health_zee_layer, false);
    layer_set_hidden(health_foot_layer, true);
    layer_set_hidden(health_foot2_layer, true);
  } else {
    //steps
    format_commas(s_steps, str);
    snprintf(str2, sizeof(str2), "%s", str);

    layer_set_hidden(health_zee_layer, true);
    layer_set_hidden(health_foot_layer, false);
    layer_set_hidden(health_foot2_layer, false);
  }
  text_layer_set_text(health_text_layer, str2);

  // Heart rate readout. 0 = no sensor reading yet; hide rather than display
  // a meaningless zero. HEALTH_BPM_TEXT is sized for two digits (≤99 BPM).
  if(s_bpm > 0 && s_bpm <= 99) {
    static char bpm_str[4];
    snprintf(bpm_str, sizeof(bpm_str), "%d", (int)s_bpm);
    text_layer_set_text(health_bpm_layer, bpm_str);
    layer_set_hidden(text_layer_get_layer(health_bpm_layer), false);
    layer_set_hidden(health_bpm_icon_layer, false);
  }
  else {
    layer_set_hidden(text_layer_get_layer(health_bpm_layer), true);
    layer_set_hidden(health_bpm_icon_layer, true);
  }
  health_layout_row();
}

/*
 * Row 1 (steps or sleep) shares the top strip with the battery cluster, so its
 * text box stops at whatever the cluster occupies:
 *
 *   PANEL_INNER_RIGHT - battery_top_reserve()
 *
 * The text is left-aligned, so the count grows rightwards into this box and is
 * clipped by it - it can no longer paint over anything, however many digits a
 * day accumulates. Row 2 (heart rate) sits below on its own row and needs no
 * re-anchoring at all.
 *
 * Called on every health update and whenever the strip's geometry changes.
 */
// Last-applied text width; reset on every health_init so a settings-driven
// deinit/re-init cycle can't skip re-bounding the rebuilt layer.
static int16_t last_text_w = -1;

void health_layout_row(void) {
  if (!health_enabled) {
    return;
  }
  int16_t text_w = PANEL_INNER_RIGHT - battery_top_reserve() - HEALTH_LEFT - HEALTH_TEXT_X;
  if (text_w != last_text_w) {
    GRect text = layer_get_frame(text_layer_get_layer(health_text_layer));
    text.size.w = text_w;
    layer_set_frame(text_layer_get_layer(health_text_layer), text);
    last_text_w = text_w;
  }
}

void health_handler(HealthEventType event, void *context) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "health_handler");
  if (event != HealthEventSleepUpdate) {
    s_steps = health_service_sum_today(HealthMetricStepCount);
  }
  if (event != HealthEventMovementUpdate ) {
    s_sleep = health_service_sum_today(HealthMetricSleepSeconds);
  }
  if (event == HealthEventHeartRateUpdate || event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    s_bpm = health_service_peek_current_value(HealthMetricHeartRateBPM);
  }
  health_update();

}

void health_init() {

  // Fresh layer set: force health_layout_row to re-bound the steps text.
  last_text_w = -1;

  if(!global_settings.Health) {
    return;
  }

  health_enabled = true;

  health_layer = layer_create(HEALTH_LAYER);
  layer_add_child(my_window_layer, health_layer);

  health_text_layer = text_layer_create_detailed(HEALTH_TEXT_LAYER,
                                GColorClear, color_helper(colors[c_t2], global_settings.Invert),
                                GTextAlignmentLeft, font_tiny);
  layer_add_child(health_layer, text_layer_get_layer(health_text_layer));

  foot_path_ptr = gpath_create(&HealthFootPathInfo);
  heel_path_ptr = gpath_create(&HealthHeelPathInfo);
  zee1_path_ptr = gpath_create(&HealthZee1PathInfo);
  zee2_path_ptr = gpath_create(&HealthZee2PathInfo);
  zee3_path_ptr = gpath_create(&HealthZee3PathInfo);

  heart_path_ptr = gpath_create(&HealthHeartPathInfo);

  // Native Emery bounds leave room for the complete zzz path.
  health_zee_layer = layer_create(GRect(0, 3, 17, 10));
  layer_set_update_proc(health_zee_layer, health_icon_layer_update_callback);
  layer_add_child(health_layer, health_zee_layer);

  health_foot_layer = layer_create(GRect(3,0,9,12));
  layer_set_update_proc(health_foot_layer, health_icon_layer_update_callback);
  layer_add_child(health_layer, health_foot_layer);

  health_foot2_layer = layer_create(GRect(8,3,9,12));
  layer_set_update_proc(health_foot2_layer, health_icon_layer_update_callback);
  layer_add_child(health_layer, health_foot2_layer);

  health_bpm_row_layer = layer_create(HEALTH_BPM_ROW);
  layer_add_child(my_window_layer, health_bpm_row_layer);

  health_bpm_icon_layer = layer_create(HEALTH_BPM_ICON);
  layer_set_update_proc(health_bpm_icon_layer, health_bpm_icon_layer_update_callback);
  layer_add_child(health_bpm_row_layer, health_bpm_icon_layer);

  health_bpm_layer = text_layer_create_detailed(HEALTH_BPM_TEXT,
                                GColorClear, color_helper(colors[c_t2], global_settings.Invert),
                                GTextAlignmentLeft, font_tiny);
  layer_add_child(health_bpm_row_layer, text_layer_get_layer(health_bpm_layer));

  health_service_events_subscribe(health_handler, NULL);
	health_handler(HealthEventMovementUpdate, NULL);
	health_handler(HealthEventSleepUpdate, NULL);

  settings_register_callback(health_settings_callback, SETTINGS_CALLBACK_HEALTH);
}

void health_deinit() {

  if(!health_enabled) {
      return;
  }

  health_service_events_unsubscribe();

  gpath_destroy(foot_path_ptr);
  foot_path_ptr = NULL;

  gpath_destroy(heel_path_ptr);
  heel_path_ptr = NULL;

  layer_destroy(health_foot_layer);
  layer_destroy(health_foot2_layer);
  layer_destroy(health_zee_layer);
  text_layer_destroy(health_text_layer);
  layer_destroy(health_layer);

  gpath_destroy(heart_path_ptr);
  heart_path_ptr = NULL;
  layer_destroy(health_bpm_icon_layer);
  health_bpm_icon_layer = NULL;
  text_layer_destroy(health_bpm_layer);
  health_bpm_layer = NULL;
  layer_destroy(health_bpm_row_layer);
  health_bpm_row_layer = NULL;

  health_enabled = false;

}

#endif
