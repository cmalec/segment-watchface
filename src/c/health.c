#include <pebble.h>
#include "_globals.h"
#include "health.h"
#include "vector.h"
#include "settings.h"
#include "helpers.h"
#include "window.h"
#include "fonts.h"

static bool health_enabled = false;

#ifdef PBL_HEALTH

static TextLayer *health_text_layer;
static Layer *health_layer, *health_foot_layer, *health_foot2_layer, *health_zee_layer, *health_bpm_icon_layer;
static TextLayer *health_bpm_layer;

// GPaths are created fresh from the pristine arrays in vector.c on every
// init and scaled exactly once — see vector.h for the double-scale story.

static GPath *foot_path_ptr = NULL;
static GPath *heel_path_ptr = NULL;
static GPath *zee1_path_ptr = NULL;
static GPath *zee2_path_ptr = NULL;
static GPath *zee3_path_ptr = NULL;

// Heart outline, used on emery (Pebble Time 2 has a HRM).
static GPath *heart_path_ptr = NULL;

static HealthValue s_sleep, s_deep_sleep, s_steps, s_active, s_distance;
#ifdef PBL_PLATFORM_EMERY
static HealthValue s_bpm;
#endif

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

#ifdef PBL_PLATFORM_EMERY
static void health_bpm_icon_layer_update_callback(Layer *my_layer, GContext* ctx) {
  graphics_context_set_stroke_color(ctx, color_helper(colors[c_t2], global_settings.Invert));
  graphics_context_set_fill_color(ctx, color_helper(colors[c_t2], global_settings.Invert));
  gpath_draw_filled(ctx, heart_path_ptr);
  gpath_draw_outline(ctx, heart_path_ptr);
}
#endif

void health_settings_callback() {
  health_deinit();

  #if defined (PBL_HEALTH)
    health_init();
  #endif
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

    #if defined (PBL_ROUND)
      GRect r = layer_get_frame(health_layer);
      if(hours == 0 && minutes < 10) {
        r.origin.x = 76;
      } else if(hours == 0) {
        r.origin.x = 73;
      } else if(hours < 10 && minutes < 10) {
        r.origin.x = 67;
      } else {
        r.origin.x = 62;
      }
      layer_set_frame(health_layer, r);
    #endif

    layer_set_hidden(health_zee_layer, false);
    layer_set_hidden(health_foot_layer, true);
    layer_set_hidden(health_foot2_layer, true);
  } else {
    //steps
    format_commas(s_steps, str);
    snprintf(str2, sizeof(str2), "%s", str);

    #if defined (PBL_ROUND)
      GRect r = layer_get_frame(health_layer);
      if(s_steps < 1000) {
        r.origin.x = 71;
      } else if(s_steps < 10000) {
        r.origin.x = 66;
      } else {
        r.origin.x = 61;
      }
      layer_set_frame(health_layer, r);
    #endif

    layer_set_hidden(health_zee_layer, true);
    layer_set_hidden(health_foot_layer, false);
    layer_set_hidden(health_foot2_layer, false);
  }
  text_layer_set_text(health_text_layer, str2);

  #ifdef PBL_PLATFORM_EMERY
  // Heart rate readout. 0 = no reading from the sensor yet; hide rather
  // than display a meaningless zero. HEALTH_BPM_TEXT is sized for two
  // digits (≤99 BPM); three digits are clipped by design rather than
  // overlapping.
  if(s_bpm > 0 && s_bpm <= 99) {
    static char bpm_str[4];
    snprintf(bpm_str, sizeof(bpm_str), "%d", (int)s_bpm);
    text_layer_set_text(health_bpm_layer, bpm_str);
    layer_set_hidden(text_layer_get_layer(health_bpm_layer), false);
    layer_set_hidden(health_bpm_icon_layer, false);
  }
  else {
    // No reading (0) or above display range: hide instead of garbage.
    layer_set_hidden(text_layer_get_layer(health_bpm_layer), true);
    layer_set_hidden(health_bpm_icon_layer, true);
  }
  #endif
}

void health_handler(HealthEventType event, void *context) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "health_handler");
  if (event != HealthEventSleepUpdate) {
    s_steps = health_service_sum_today(HealthMetricStepCount);
  }
  if (event != HealthEventMovementUpdate ) {
    s_sleep = health_service_sum_today(HealthMetricSleepSeconds);
  }
  #ifdef PBL_PLATFORM_EMERY
  if (event == HealthEventHeartRateUpdate || event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    s_bpm = health_service_peek_current_value(HealthMetricHeartRateBPM);
  }
  #endif
  health_update();

}

void health_init() {

  if(!global_settings.Health) {
    return;
  }

  health_layer = layer_create(HEALTH_LAYER);
  layer_add_child(my_window_layer, health_layer);

  health_text_layer = text_layer_create_detailed(HEALTH_TEXT_LAYER, false,
                                GColorClear, color_helper(colors[c_t2], global_settings.Invert),
                                GTextAlignmentLeft, font_tiny);
  layer_add_child(health_layer, text_layer_get_layer(health_text_layer));

  foot_path_ptr = vector_create(&HealthFootPathInfo);
  heel_path_ptr = vector_create(&HealthHeelPathInfo);
  zee1_path_ptr = vector_create(&HealthZee1PathInfo);
  zee2_path_ptr = vector_create(&HealthZee2PathInfo);
  zee3_path_ptr = vector_create(&HealthZee3PathInfo);

  #if defined(PBL_PLATFORM_EMERY)
  heart_path_ptr = vector_create(&HealthHeartPathInfo);
  #endif

  health_zee_layer = layer_create(GRect(0,3,14,7));
  layer_set_update_proc(health_zee_layer, health_icon_layer_update_callback);
  layer_add_child(health_layer, health_zee_layer);

  health_foot_layer = layer_create(GRect(3,0,9,12));
  layer_set_update_proc(health_foot_layer, health_icon_layer_update_callback);
  layer_add_child(health_layer, health_foot_layer);

  health_foot2_layer = layer_create(GRect(8,3,9,12));
  layer_set_update_proc(health_foot2_layer, health_icon_layer_update_callback);
  layer_add_child(health_layer, health_foot2_layer);

  #ifdef PBL_PLATFORM_EMERY
  health_bpm_icon_layer = layer_create(HEALTH_BPM_ICON);
  layer_set_update_proc(health_bpm_icon_layer, health_bpm_icon_layer_update_callback);
  layer_add_child(health_layer, health_bpm_icon_layer);

  health_bpm_layer = text_layer_create_detailed(HEALTH_BPM_TEXT, false,
                                GColorClear, color_helper(colors[c_t2], global_settings.Invert),
                                GTextAlignmentRight, font_tiny);
  layer_add_child(health_layer, text_layer_get_layer(health_bpm_layer));
  #endif

  health_service_events_subscribe(health_handler, NULL);
	health_handler(HealthEventMovementUpdate, NULL);
	health_handler(HealthEventSleepUpdate, NULL);

  settings_register_callback(health_settings_callback, SETTINGS_CALLBACK_HEALTH);

  health_enabled = true;
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

  #ifdef PBL_PLATFORM_EMERY
  gpath_destroy(heart_path_ptr);
  heart_path_ptr = NULL;
  layer_destroy(health_bpm_icon_layer);
  text_layer_destroy(health_bpm_layer);
  #endif

  health_enabled = false;

}

#endif

int16_t health_bpm_text_offset_x() {
  // Health text box right edge, BPM text starts this far to its right.
  #ifdef PBL_PLATFORM_EMERY
    return 22;
  #else
    return 0;
  #endif
}
