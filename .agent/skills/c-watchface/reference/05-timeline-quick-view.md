# Part 5 — Timeline Quick View (UnobstructedArea)

**Goal:** when the system overlay covers the bottom ~51 px, reposition time/date/weather into the remaining space instead of hiding behind it.

## API

```c
GRect full_bounds = layer_get_bounds(s_window_layer);            // whole screen
GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);  // minus overlays
```

Keep the root layer around — handlers need it:

```c
static Layer *s_window_layer;

// first line of main_window_load():
s_window_layer = window_get_root_layer(window);
```

Three handlers, three different signatures (common SDK pattern):

| Handler | Fires | Typical use |
|---|---|---|
| `.will_change` | once before the animation, receives the **final** unobstructed `GRect` | prepare / hide clutter |
| `.change` | repeatedly during the animation (`AnimationProgress`) | reposition layers every frame |
| `.did_change` | once after the animation | settle final state |

## Implementation

```c
static void prv_reposition_layers(GRect bounds) {
  int block_height = 56 + 30;
  int time_y = (bounds.size.h / 2) - (block_height / 2) - 10;
  int date_y = time_y + 56;
  int weather_y = bounds.size.h - PBL_IF_ROUND_ELSE(40, 30);

  GRect frame = layer_get_frame(text_layer_get_layer(s_time_layer));
  frame.origin.y = time_y;
  layer_set_frame(text_layer_get_layer(s_time_layer), frame);

  frame = layer_get_frame(text_layer_get_layer(s_date_layer));
  frame.origin.y = date_y;
  layer_set_frame(text_layer_get_layer(s_date_layer), frame);

  frame = layer_get_frame(text_layer_get_layer(s_weather_layer));
  frame.origin.y = weather_y;
  layer_set_frame(text_layer_get_layer(s_weather_layer), frame);
}

static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area, void *context) {
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), true);   // no clutter mid-transition
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  prv_reposition_layers(layer_get_unobstructed_bounds(s_window_layer));
}

static void prv_unobstructed_did_change(void *context) {
  GRect full_bounds = layer_get_bounds(s_window_layer);
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  bool obstructed = !grect_equal(&full_bounds, &bounds);

  // Show the BT icon only when there is room and the phone is disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer),
                   obstructed || connection_service_peek_pebble_app_connection());
}
```

Sharing the same centering formula with `main_window_load()` is what makes the block slide smoothly: as the unobstructed height shrinks, the computed `time_y` follows.

## Startup state

The handlers only fire on *change*, so a watchface launched while Quick View is already open would render in the wrong place. Apply the layout once manually before subscribing:

```c
prv_update_display();                 // part 6
prv_unobstructed_change(0, NULL);     // place layers for the current state
prv_unobstructed_did_change(NULL);    // icon visibility for the current state

UnobstructedAreaHandlers handlers = {
  .will_change = prv_unobstructed_will_change,
  .change = prv_unobstructed_change,
  .did_change = prv_unobstructed_did_change
};
unobstructed_area_service_subscribe(handlers, NULL);
```

Unsubscribe in the unload handler:

```c
unobstructed_area_service_unsubscribe();
```

## Verify

```bash
pebble build && pebble install --emulator emery
pebble emu-set-timeline-quick-view on --emulator emery
pebble screenshot --no-open --emulator emery shot-05-on.png
pebble emu-set-timeline-quick-view off --emulator emery
pebble screenshot --no-open --emulator emery shot-05-off.png
```

On: time/date/weather compressed into the visible band, BT icon hidden. Off: original layout, icon back if disconnected. Read both screenshots.

**The emulator's Quick View toggle is sticky across installs** — if the layout looks wrong right after a fresh install, check whether Quick View is still on. The toggle can also need re-issuing and a few seconds.

## Gotchas

- Quick View is not supported on round platforms (chalk, gabbro); the handler code still runs harmlessly.
- Don't hide content permanently — reposition it. Users expect the time to stay visible.
- `grect_equal()` takes pointers; compare against `layer_get_bounds()`, not a cached rect, if the window can resize.

Source: <https://developer.repebble.com/tutorials/watchface-tutorial/part5/>
