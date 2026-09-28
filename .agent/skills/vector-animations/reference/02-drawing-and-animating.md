# Drawing images and animating sequences in C

## One image, one call

```c
static GDrawCommandImage *s_command_image;

// init(), before window_stack_push():
s_command_image = gdraw_command_image_create_with_resource(RESOURCE_ID_WEATHER_IMAGE);

static void update_proc(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_command_image, GPoint(10, 20));   // origin, not bounds
}

// main_window_unload():
gdraw_command_image_destroy(s_command_image);
```

The layer's frame must cover the image's `GSize` (query with `gdraw_command_image_get_bounds_size()`).

## Sequences: frames on a timer

```c
static GDrawCommandSequence *s_command_seq;
static Layer *s_canvas_layer;
static uint32_t s_index;          // uint32_t — the frame APIs are unsigned
static AppTimer *s_timer;
static int s_loops;

#define PLAY_LOOPS 2

static void prv_next_frame(void *context) {
  s_timer = NULL;
  layer_mark_dirty(s_canvas_layer);

  if (++s_index >= gdraw_command_sequence_get_num_frames(s_command_seq)) {
    s_index = 0;
    if (++s_loops >= PLAY_LOOPS) return;                     // stop: battery
  }
  s_timer = app_timer_register(FRAME_MS, prv_next_frame, NULL);
}

static void update_proc(Layer *layer, GContext *ctx) {
  GDrawCommandFrame *frame =
      gdraw_command_sequence_get_frame_by_index(s_command_seq, s_index);
  if (frame) {
    gdraw_command_frame_draw(ctx, s_command_seq, frame, GPoint(0, 30));
  }
}
```

Frame interval straight from the asset (the tool writes 33 ms by default):

```c
uint32_t frames = gdraw_command_sequence_get_num_frames(s_command_seq);
int frame_ms = frames ? (int)(gdraw_command_sequence_get_total_duration(s_command_seq) / frames) : 33;
```

### Alternative: let the SDK pick the frame

```c
GDrawCommandFrame *frame =
    gdraw_command_sequence_get_frame_by_elapsed(s_command_seq, elapsed_ms_since_start);
```

Use this when the animation must stay in sync with a clock (progress indicators, spinners tied to time) instead of your own counter.

### Playing on demand

```c
static void tap_handler(AccelAxisType axis, int32_t direction) {
  prv_start_animation();     // reset index + loop counter, kick the timer
}

accel_tap_service_subscribe(tap_handler);   // watchfaces: tap is the only input
```

Watchapps can use the click API instead (`window_set_click_config_provider`) to start/stop playback with a button.

## Frame/sequence accessors

| Call | Use |
|---|---|
| `gdraw_command_sequence_get_num_frames()` | frame count |
| `gdraw_command_sequence_get_total_duration()` | sum of frame durations, ms |
| `gdraw_command_sequence_get_frame_by_index()` | frame for an index (NULL if out of range) |
| `gdraw_command_sequence_get_frame_by_elapsed()` | frame for a wall-clock offset |
| `gdraw_command_sequence_get/set_play_count()` | metadata for playback logic |
| `gdraw_command_frame_get/set_duration()` | per-frame timing |
| `gdraw_command_frame_get_command_list()` | the frame's shapes, for manipulation |

## Performance and battery

- **Frame rate:** ~30 fps (`33 ms`) is the practical ceiling; the emulator renders faster than a real watch, so keep budgets conservative. 50–100 ms steps look perfectly smooth for icons.
- **Redraw only the canvas layer** (`layer_mark_dirty(s_canvas_layer)`), not the whole window.
- **Stop the timer** when the burst ends or the window unloads; cancel it in `main_window_unload()`/`deinit()`:

```c
if (s_timer) { app_timer_cancel(s_timer); s_timer = NULL; }
```

- **Watchfaces:** prefer event-driven bursts (tap/minute change) over continuous loops.
- **Watchapps:** animation may run while the app is visible, but stop it on `window_disappear` — a game or menu behind the app must not keep burning cycles.

## Verify

```bash
pebble build && pebble install --emulator emery
pebble screenshot --no-open --emulator emery frame1.png
pebble emu-tap --emulator emery
pebble screenshot --no-open --emulator emery frame2.png     # must differ from frame1
```

For a burst that is too short to catch, temporarily raise `PLAY_LOOPS` in a scratch copy, screenshot every second, and confirm the frames advance in order. Remember that `pebble screenshot` is slow relative to a 33 ms frame interval — the point is only to prove different frames render.

## Gotchas

- `-Werror=sign-compare`: a plain `int` index versus `gdraw_command_sequence_get_num_frames()` fails the build. Use `uint32_t`.
- Drawing must happen inside a `LayerUpdateProc`; drawing from a timer callback directly will not paint.
- Forgetting `layer_mark_dirty()` leaves the canvas on its previous frame.
- `gdraw_command_frame_draw()` needs the *sequence* plus the frame — passing the frame alone is not the API.
- Destroy both the sequence and the image, and cancel the timer, or the app leaks on every window load.

Source: <https://developer.repebble.com/tutorials/advanced/vector-animations/>
