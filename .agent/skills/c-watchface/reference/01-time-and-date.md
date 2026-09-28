# Part 1 — Your First Watchface (Window, TextLayer, TickTimerService)

**Goal:** black-background watchface showing `HH:MM` and `Mon Jan 01`, updated once a minute.
**Produces:** a window with load/unload handlers and two text layers.

## Project

```bash
pebble new-project --simple watchface     # empty project
```

`package.json`:

```json
"watchapp": { "watchface": true }
```

Watchfaces are the default watch display: Up/Down belong to the timeline and Select to the launcher, so **a watchface gets no button input**. Interactive apps set `"watchface": false` and use the click API.

## App skeleton

```c
#include <pebble.h>

static Window *s_main_window;

static void main_window_load(Window *window) {

}

static void main_window_unload(Window *window) {

}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
```

`main()` → `init()` sets things up, `app_event_loop()` waits for system events, `deinit()` cleans up. Prefix file-scoped statics with `s_`.

## Time

```c
static TextLayer *s_time_layer;

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
}
```

## TickTimerService

```c
static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];     // static: the layer keeps the pointer, not a copy
  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// in init(), after window_stack_push():
update_time();                                        // draw immediately
tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
```

`MINUTE_UNIT` fires every minute. `SECOND_UNIT` exists but costs battery — only for an explicitly requested seconds display.

## Date

```c
static TextLayer *s_date_layer;

s_date_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(110, 104), bounds.size.w, 30));
text_layer_set_background_color(s_date_layer, GColorClear);
text_layer_set_text_color(s_date_layer, GColorWhite);
text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

// add to update_time():
static char s_date_buffer[16];
strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);   // "Mon Jan 01"
text_layer_set_text(s_date_layer, s_date_buffer);
```

## Verify

```bash
pebble build && pebble install --emulator emery
pebble screenshot --no-open --emulator emery shot-01.png   # read the image
pebble emu-set-time --emulator emery ...                   # proves the redraw path
```

## Gotchas

- Watchfaces cannot use Up/Down; if you need input, build a watchapp.
- Text buffers must be `static` (or otherwise live as long as the layer).
- Use `layer_get_bounds()` rather than hardcoding sizes: 144x168 (aplite/basalt/diorite/flint), 180x180 (chalk), 200x228 (emery), 260x260 (gabbro).
- `PBL_IF_ROUND_ELSE(round, rect)` is how one binary serves round and rectangular screens.

Source: <https://developer.repebble.com/tutorials/watchface-tutorial/part1/>
