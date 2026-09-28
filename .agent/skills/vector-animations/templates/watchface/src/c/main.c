// Vector watchface — PDC (Pebble Draw Command) image + sequence.
//
//   • a static PDC image (sun) drawn with one gdraw_command_image_draw() call
//   • a PDC sequence (rotating triangle) animated frame by frame
//
// Battery discipline: the sequence plays PLAY_LOOPS times at launch and then
// the frame timer is cancelled; tap the watch to replay it. A watchface that
// animates forever costs battery for nothing.
//
// PDC works on every current SDK target — verified rendering on emery, gabbro
// (round, colour) and aplite (B&W: shapes render white/dithered). The tutorial's
// "Basalt only" notice is out of date.

#include <pebble.h>

#define PLAY_LOOPS 2

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static GDrawCommandImage *s_sun_image;
static GDrawCommandSequence *s_spinner;

static uint32_t s_frame_index;      // PDC frame APIs use uint32_t — a signed index trips -Werror=sign-compare
static int s_loops_done;
static int s_frame_ms = 50;
static AppTimer *s_frame_timer;

// ---------------------------------------------------------------- animation

static void prv_stop_animation() {
  if (s_frame_timer) {
    app_timer_cancel(s_frame_timer);
    s_frame_timer = NULL;
  }
}

static void prv_next_frame(void *context) {
  s_frame_timer = NULL;
  layer_mark_dirty(s_canvas_layer);

  s_frame_index++;
  if (s_frame_index >= gdraw_command_sequence_get_num_frames(s_spinner)) {
    s_frame_index = 0;
    if (++s_loops_done >= PLAY_LOOPS) {
      return;                       // playback finished: no timer until the next tap
    }
  }
  s_frame_timer = app_timer_register(s_frame_ms, prv_next_frame, NULL);
}

static void prv_start_animation() {
  prv_stop_animation();
  s_frame_index = 0;
  s_loops_done = 0;
  prv_next_frame(NULL);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  prv_start_animation();
}

// ---------------------------------------------------------------- drawing

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Static vector image: one call, real point data
  gdraw_command_image_draw(ctx, s_sun_image, GPoint(6, 6));

  // Current frame of the sequence, positioned from the right edge
  GDrawCommandFrame *frame = gdraw_command_sequence_get_frame_by_index(s_spinner, s_frame_index);
  if (frame) {
    gdraw_command_frame_draw(ctx, s_spinner, frame, GPoint(bounds.size.w - 66, 6));
  }
}

// ---------------------------------------------------------------- time

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// ---------------------------------------------------------------- window

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  s_time_layer = text_layer_create(GRect(0, (bounds.size.h / 2) - 34, bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_date_layer = text_layer_create(GRect(0, (bounds.size.h / 2) + 18, bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  update_time();

  // Frame interval follows the sequence's own frame duration
  uint32_t frames = gdraw_command_sequence_get_num_frames(s_spinner);
  if (frames > 0) {
    s_frame_ms = (int)(gdraw_command_sequence_get_total_duration(s_spinner) / frames);
    if (s_frame_ms < 10) {
      s_frame_ms = 10;
    }
  }
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  layer_destroy(s_canvas_layer);
}

// ---------------------------------------------------------------- app

static void init() {
  // Load the vector resources before the window draws
  s_sun_image = gdraw_command_image_create_with_resource(RESOURCE_ID_PDC_SUN);
  s_spinner = gdraw_command_sequence_create_with_resource(RESOURCE_ID_PDC_SPINNER);

  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorOxfordBlue);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(tap_handler);

  prv_start_animation();
}

static void deinit() {
  prv_stop_animation();
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();

  gdraw_command_image_destroy(s_sun_image);
  gdraw_command_sequence_destroy(s_spinner);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
