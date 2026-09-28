---
name: vector-animations
description: Create and animate Pebble Draw Command (PDC) vector assets — SVG-to-PDC conversion, PDC images and frame sequences, runtime point/colour manipulation, frame timers, and the Alloy/Poco equivalents. Use whenever a Pebble app or watchface needs vector icons, scalable graphics, or multi-frame animation, or when working from the official Vector Animations tutorial.
---

# Pebble Vector Animations (PDC)

PDC ("Pebble Draw Command") files are **vector** assets: point lists plus fill/stroke instructions instead of per-pixel data. A PDC file holds either one image (`PDCI`) or a sequence of frames (`PDCS`). They are drawn through the same `GContext` as bitmaps and can be manipulated point-by-point at runtime.

**Use vectors when:** icons/simple shapes, anywhere you need scaling or per-point animation, or long frame animations that would be huge as PNGs. **Use bitmaps when:** photos, gradients, or complex artwork — PDC rendering cost grows with point count.

Everything below is verified on pebble tool v5.0.40 / SDK 4.33.1 (assets generated here were rendered on the emery, gabbro and aplite emulators).

> The tutorial's "Basalt only" platform notice is **out of date**: PDC built and rendered on aplite (monochrome, dithered), emery and gabbro in this skill's verification. Colour platforms render the declared colours; aplite/diorite fall back to white/dither.

## Fast path — animated watchface from the verified template

```bash
SKILL=<this skill dir>
cp -R "$SKILL/templates/watchface" my-face && cd my-face
uuidgen                        # paste into package.json → pebble.uuid
pebble build                   # all 7 platforms; PDC assets are already committed
pebble install --emulator emery
pebble screenshot --no-open --emulator emery shot.png      # then READ the image
```

The template draws a PDC **image** (sun) and plays a PDC **sequence** (rotating triangle) in a short burst at launch, then stops the frame timer; tap the watch to replay. Assets regenerate with:

```bash
python3 "$SKILL/scripts/make_demo_assets.py" .
```

## Tooling — two routes to a `.pdc`

| Route                      | Tool                  | When                                                                     |
|----------------------------|-----------------------|--------------------------------------------------------------------------|
| Programmatic (recommended) | `scripts/make_pdc.py` | You are generating shapes/frames in code or JSON; no dependencies at all |
| SVG (designer assets)      | `scripts/svg2pdc.py`  | You have `.svg` files from Illustrator/Inkscape                          |

**`make_pdc.py`** is a library + CLI:

```bash
# library
from make_pdc import argb8, circle, image, path, sequence
open("sun.pdc", "wb").write(image(60, 60, [circle((30, 30), 9, fill=argb8(255, 170, 0))]))

# JSON spec (image or sequence; see the SPEC docstring)
python3 make_pdc.py spec.json -o resources/pdcs/spinner.pdc
```

**`svg2pdc.py`** is the upstream Pebble tool (MIT, © 2015 Pebble Technology) **ported to Python 3** — the published version is Python 2 and will not run on a modern interpreter:

```bash
python3 scripts/svg2pdc.py image.svg                  # -> image.pdc
python3 scripts/svg2pdc.py --sequence frames/         # -> frames.pdc (33 ms frames, 1 play)
python3 scripts/svg2pdc.py --sequence -d 50 -c 3 frames/   # 50 ms per frame, 3 plays
```

Port notes: Python 3 syntax, little-endian struct packing, binary output, `svg.path` imported lazily, and the SDK's `pebble_image_routines` dependency replaced by a local palette conversion. SVG elements supported: `g`, `layer`, `path`\*, `rect`, `polyline`, `polygon`, `line`, `circle`. \*`<path>` needs `pip install svg-path`; every other element works with zero dependencies (verified: rect + circle + polygon render exactly as specified).

## Wiring resources

`package.json` — PDC files are declared as `raw` resources and must live under `resources/`:

```json
"resources": {
  "media": [
    {
      "type": "raw",
      "name": "PDC_SUN",
      "file": "pdcs/sun.pdc"
    },
    {
      "type": "raw",
      "name": "PDC_SPINNER",
      "file": "pdcs/spinner.pdc"
    }
  ]
}
```

`name` becomes `RESOURCE_ID_PDC_SUN` in C. There is no build-time SVG conversion in the SDK: the `.pdc` file is what ships.

## C API

```c
static GDrawCommandImage *s_image;
static GDrawCommandSequence *s_sequence;

// load before the window draws (init(), before window_stack_push)
s_image = gdraw_command_image_create_with_resource(RESOURCE_ID_PDC_SUN);
s_sequence = gdraw_command_sequence_create_with_resource(RESOURCE_ID_PDC_SPINNER);

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  gdraw_command_image_draw(ctx, s_image, GPoint(6, 6));       // one call, one image

  GDrawCommandFrame *frame =
      gdraw_command_sequence_get_frame_by_index(s_sequence, s_frame_index);
  if (frame) {
    gdraw_command_frame_draw(ctx, s_sequence, frame, GPoint(bounds.size.w - 66, 6));
  }
}

// destroy in main_window_unload()/deinit()
gdraw_command_image_destroy(s_image);
gdraw_command_sequence_destroy(s_sequence);
```

Frame stepping with a timer (and stopping when the burst is over — see the battery rule below):

```c
static uint32_t s_frame_index;          // uint32_t: the PDC frame APIs return uint32_t
static int s_loops_done;
static int s_frame_ms = 50;
static AppTimer *s_frame_timer;

static void prv_next_frame(void *context) {
  s_frame_timer = NULL;
  layer_mark_dirty(s_canvas_layer);                       // draw the current index

  if (++s_frame_index >= gdraw_command_sequence_get_num_frames(s_sequence)) {
    s_frame_index = 0;
    if (++s_loops_done >= PLAY_LOOPS) return;             // done: no timer until the next tap
  }
  s_frame_timer = app_timer_register(s_frame_ms, prv_next_frame, NULL);
}
```

Useful sequence accessors: `gdraw_command_sequence_get_num_frames()`, `get_total_duration()`, `get_play_count()`, `get_bounds_size()`, and `gdraw_command_sequence_get_frame_by_elapsed()` when you want the frame for a wall-clock time instead of keeping your own index.

### Battery rule

A watchface that animates forever burns battery for nothing. Battery-safe pattern (what the template does):

- play a **burst** of 1–3 loops at launch, then cancel the frame timer;
- restart the animation on **input** (tap via `accel_tap_service_subscribe`) or on a meaningful event;
- if the animation must run continuously, prefer ~30 fps (`33 ms`) and stop it when the window is not visible (`main_window_unload`).

## Runtime manipulation

Every command in an image/sequence can be inspected and changed after loading — this is the "vector" payoff:

```c
GDrawCommandImage *clone = gdraw_command_image_clone(s_image);      // edit a copy, keep the resource pristine
GDrawCommandList *list = gdraw_command_image_get_command_list(clone);
uint32_t count = gdraw_command_list_get_num_commands(list);

for (uint32_t i = 0; i < count; i++) {
  GDrawCommand *command = gdraw_command_list_get_command(list, i);
  gdraw_command_set_fill_color(command, GColorRed);
  gdraw_command_set_stroke_color(command, GColorWhite);
  gdraw_command_set_stroke_width(command, 2);
  gdraw_command_set_hidden(command, false);
  gdraw_command_set_point(command, 0, GPoint(10, 10));   // point 0 *within* this command
}

gdraw_command_image_set_bounds_size(clone, GSize(80, 80));   // scale the drawing box
```

Sequence equivalents: `gdraw_command_sequence_clone()`, `gdraw_command_sequence_get_frame_by_index()`, `gdraw_command_sequence_get_play_count()` / `set_play_count()`, `gdraw_command_frame_get_duration()` / `set_duration()`, `gdraw_command_frame_get_command_list()`.

## Alloy (Poco) equivalent

Alloy exposes the same PDC files through Poco — `Poco.PebbleDrawCommandImage` / `Poco.PebbleDrawCommandSequence` with `render.drawDCI(dci, x, y)`, `.scale()`, `.rotate()`, `.clone()`, and a settable `sequence.time`. Note that the constructor runs `Number(id)`, so it wants the resource's **numeric id**, not a filename; that route was not run here — see [reference/04-alloy-pdc.md](reference/04-alloy-pdc.md) for the exact API and the caveat.

## Verifying — what actually works in practice

1. **Read the rendered screenshots.** PDC draws silently: a malformed file renders nothing and a crash is invisible in logs here (`pebble logs` produced no output in this environment).
2. **Prove a sequence animates** by taking multiple screenshots while it plays — with a short burst the default 2 loops (e.g. 1.2 s) are easy to miss, so verify with a scratch build that raises `PLAY_LOOPS` (e.g. 40) and screenshot ~1 s apart; frames must differ.
3. **Installing a new watchface does not always make it the active watchface.** If the screenshot still shows the previous app (or the launcher says "Install an app to continue"), wipe and reinstall: `printf 'y\n' | pebble wipe` then `pebble install --emulator emery`. Reinstalling the *same* UUID stays in place, which is the easiest way to iterate when the emulator already shows your face.
4. **Check the platforms you claim**: `pebble install --emulator aplite` / `gabbro` with a screenshot each.

```bash
pebble build                                     # every platform in targetPlatforms
pebble install --emulator emery                  # + aplite / gabbro as needed
pebble screenshot --no-open --emulator emery shot.png
pebble emu-tap --emulator emery                  # restart the animation burst
```

## Pitfalls

| Symptom                                                                  | Cause / fix                                                                                                                                                                                      |
|--------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Nothing renders, no error                                                | Resource not declared (`"type": "raw"`), file not under `resources/`, wrong `RESOURCE_ID_*`, or a malformed PDC — dump the first bytes (`PDCI`/`PDCS` magic) and compare with a known-good file. |
| Build fails: `comparison of integer expressions of different signedness` | The PDC frame APIs return `uint32_t`; declare frame indexes/counters as `uint32_t` (the SDK builds with `-Werror=sign-compare`).                                                                 |
| Nothing on the emulator after install                                    | Another watchface is active — wipe and reinstall (see Verifying #3).                                                                                                                             |
| Shapes drawn at the wrong place                                          | `gdraw_command_image_draw(ctx, image, origin)` takes the **origin**, not the layer bounds; the layer must cover the drawing area.                                                                |
| First frame only, never advances                                         | `layer_mark_dirty()` missing in the timer callback, or the timer was never started/restarted.                                                                                                    |
| Animation never stops                                                    | Timer keeps re-registering — add a loop counter and stop, and `app_timer_cancel()` in unload/deinit.                                                                                             |
| Colours look wrong                                                       | PDC colours are Pebble 8-bit **ARGB2222**; `argb8(r, g, b, a)` truncates to 2 bits per channel.                                                                                                  |
| Aplite looks plain                                                       | Expected: 1-bit display, colours collapse to white/dither.                                                                                                                                       |
| Upstream tool crashes on Python 3                                        | Use the ported `scripts/svg2pdc.py` in this skill, not the original from `pebble-examples`.                                                                                                      |
| `svg2pdc` fails on `<path>`                                              | `pip install svg-path`, or express the shape with `polygon`/`polyline` — those need no dependency.                                                                                               |
| Resource grows the app a lot                                             | PDC is compact (this skill's demo assets: 197 B and 474 B) — if a file is large, reduce point counts / frame counts.                                                                             |

## Delivery checklist

- [ ] `.pdc` files generated (script committed next to the assets) and declared as `raw` resources
- [ ] `gdraw_command_image_create_with_resource` / `gdraw_command_sequence_create_with_resource` loaded before the first draw
- [ ] Frame index/loop counters are `uint32_t`; frame timer starts, advances, and stops
- [ ] Animation restarts on input/event; timer cancelled in unload/deinit
- [ ] All `*_create_with_resource` matched with a `*_destroy`
- [ ] `pebble build` clean for every target platform
- [ ] Screenshots read on at least one colour platform (+ aplite or gabbro if claimed); multiple frames compared to prove animation
