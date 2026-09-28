# Runtime manipulation — the vector payoff

Once loaded, every PDC command can be inspected and edited in place. This is what bitmaps cannot do: move individual points, recolour, hide, rescale — without shipping extra assets.

## Command lists

```c
GDrawCommandImage *clone = gdraw_command_image_clone(s_image);   // edit a copy; the resource stays pristine
GDrawCommandList *list = gdraw_command_image_get_command_list(clone);
uint32_t count = gdraw_command_list_get_num_commands(list);

for (uint32_t i = 0; i < count; i++) {
  GDrawCommand *command = gdraw_command_list_get_command(list, i);

  GDrawCommandType type = gdraw_command_get_type(command);       // path / precise path / circle
  uint32_t points = gdraw_command_get_num_points(command);
  GPoint point = gdraw_command_get_point(command, 0);
  GColor fill = gdraw_command_get_fill_color(command);
  uint8_t stroke_width = gdraw_command_get_stroke_width(command);
  bool hidden = gdraw_command_get_hidden(command);
  bool open = gdraw_command_get_path_open(command);              // polyline vs polygon
  uint16_t radius = gdraw_command_get_radius(command);           // circles only
  (void)type; (void)points; (void)point; (void)fill;
  (void)stroke_width; (void)hidden; (void)open; (void)radius;
}
```

`gdraw_command_list_iterate(list, callback, context)` walks the same list with a callback instead of an index loop.

## Editing

```c
gdraw_command_set_fill_color(command, GColorRed);
gdraw_command_set_stroke_color(command, GColorWhite);
gdraw_command_set_stroke_width(command, 2);
gdraw_command_set_hidden(command, true);
gdraw_command_set_path_open(command, false);        // close an open path → filled shape
gdraw_command_set_point(command, 0, GPoint(10, 10));// move point 0 *within this command*
gdraw_command_set_radius(command, 6);               // circles

gdraw_command_image_set_bounds_size(clone, GSize(80, 80));      // drawing box → scales the art
layer_mark_dirty(s_canvas_layer);                              // repaint after editing
```

Sequence variants: `gdraw_command_sequence_clone()`, `gdraw_command_sequence_set_bounds_size()`, `gdraw_command_sequence_get/set_play_count()`, `gdraw_command_frame_get_command_list()`, `gdraw_command_frame_get/set_duration()`.

## Patterns that work well

| Pattern | How |
|---|---|
| **Recolour by state** | Load one icon; set fill/stroke per command for battery level, weather condition, connected/disconnected. |
| **Needle / gauge** | One `path` command; recompute a point and call `gdraw_command_set_point()`, then `layer_mark_dirty()`. |
| **Progress ring or bar** | Move the endpoint point of a path per tick; cheaper than many bitmap frames. |
| **Scaling for emphasis** | `gdraw_command_image_set_bounds_size()` (or `sequence_set_bounds_size()`) to grow/shrink the art on an `Animation` progress callback. |
| **Build-up animations** | Ship all shapes in one image, start with `gdraw_command_set_hidden(cmd, true)` per command, reveal them one by one. |

## Cost and caution

- Editing happens on the watch CPU every time you call it: **clone once** (or load once), then mutate the same object; do not clone per frame.
- Each `set_*` only changes data — you must `layer_mark_dirty()` to see it.
- `gdraw_command_set_point()` indexes points **within one command**; get `num_points` first, and skip circles (they hold a single centre point).
- Clones must be destroyed with `gdraw_command_image_destroy()` / `gdraw_command_sequence_destroy()`, same as loaded resources.
- Precise-path (type 3) coordinates are stored in 1/8 px units — a point you set to `(10, 10)` lands at 10 px only for type 1 paths.

## Verify

```bash
pebble build && pebble install --emulator emery
pebble screenshot --no-open --emulator emery before.png
pebble emu-tap --emulator emery          # or whatever triggers your mutation
pebble screenshot --no-open --emulator emery after.png    # shape/colour must change
```

Observed in this skill's verification: cloning an image at load, then calling `gdraw_command_set_fill_color()` over its command list on a tap (followed by `layer_mark_dirty()`), flipped the rendered icon between green and red while the underlying resource stayed untouched.

Source: <https://developer.repebble.com/tutorials/advanced/vector-animations/> (draw command API reference: <https://developer.repebble.com/docs/c/Graphics/Draw_Commands/>)
