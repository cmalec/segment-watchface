# PDC format and tooling

## The binary format

PDC files are little-endian. Everything below was confirmed by byte-comparing this skill's generated files with Pebble's official `Pebble_50x50_*.pdc` icons.

```
file      = magic[4] + uint32 payload_size + payload
magic     = b"PDCI" (image) | b"PDCS" (sequence)

header    = uint8 version(=1) + uint8 reserved(0) + int16 width + int16 height

image     = header + command_list
sequence  = header + uint16 play_count + uint16 frame_count + frame*
frame     = uint16 duration_ms + command_list

command_list = uint16 command_count + command*
command      = uint8 type + uint8 reserved(0) + uint8 stroke_color + uint8 stroke_width
               + uint8 fill_color
               + [path]   uint8 path_open + uint8 unused
                 [circle] uint16 radius
               + uint16 point_count + (int16 x + int16 y)*

type = 1 path | 2 circle | 3 precise path (coordinates in 1/8 px units)
```

- **Colours** are Pebble 8-bit ARGB2222: `(a>>6)<<6 | (r>>6)<<4 | (g>>6)<<2 | (b>>6)` — 2 bits per channel over the 4-level palette `{0, 85, 170, 255}`. `make_pdc.argb8()` does this.
- **A circle's point list holds its centre**, so the whole shape can be moved later with `gdraw_command_set_point()`; radius has its own setter.
- **Precise paths** multiply coordinates by 8 (sub-pixel precision) and must be declared as type 3 *and* written in that scaled space.
- Frame duration is per frame, but `svg2pdc --sequence` writes the same duration to every frame; `make_pdc.sequence()` accepts per-frame durations.

## Generating files

### Route A — `scripts/make_pdc.py` (no dependencies)

```python
from make_pdc import argb8, circle, image, path, sequence

sun = image(60, 60, [
    circle((30, 30), 9, fill=argb8(255, 170, 0)),
    path([(27, 17), (33, 17), (30, 8)], fill=argb8(255, 170, 0)),
])
open("sun.pdc", "wb").write(sun)
```

Animated frames are just a list of command lists:

```python
import math
frames = []
for i in range(12):
    a = 2 * math.pi * i / 12
    pts = [(30 + 16 * math.sin(a + k * 2 * math.pi / 3),
            30 - 16 * math.cos(a + k * 2 * math.pi / 3)) for k in range(3)]
    frames.append([path(pts, fill=argb8(0, 170, 255), stroke=argb8(255, 255, 255), width=1)])
open("spinner.pdc", "wb").write(sequence(60, 60, frames, duration=50, play_count=1))
```

CLI + JSON spec, for when writing Python is inconvenient:

```json
{
  "type": "sequence",
  "width": 60, "height": 60,
  "duration": 33, "play_count": 1,
  "frames": [
    {"commands": [{"shape": "circle", "center": [30, 30], "radius": 6, "fill": [255, 170, 0]}]},
    {"commands": [{"shape": "circle", "center": [30, 30], "radius": 9, "fill": [255, 170, 0]}]}
  ]
}
```

```bash
python3 scripts/make_pdc.py spec.json -o resources/pdcs/pulse.pdc
```

### Route B — `scripts/svg2pdc.py` (SVG assets)

The upstream tool is Python 2; the copy in this skill is ported to Python 3 (same output format, verified). Element support: `g`, `layer`, `path`, `rect`, `polyline`, `polygon`, `line`, `circle`. Only `<path>` needs `pip install svg-path` — everything else is dependency-free.

```bash
python3 scripts/svg2pdc.py icon.svg -o resources/pdcs/icon.pdc
python3 scripts/svg2pdc.py --sequence frames/ -d 50 -c 3 -o resources/pdcs/spin.pdc
```

Coordinates from SVG are shifted by (−0.5, −0.5) and rounded, matching how Pebble rasterises vectors; that is why a `viewBox`-based SVG lines up with the PDC grid.

## Verifying a generated file

```bash
head -c 8 file.pdc | xxd          # magic + payload size
```

Or dump it structurally (header → command list → points) and confirm counts match the source shapes. Real-world checks from this skill's verification:

- single-shape image (`circle` + 8 ray `path`s, 60x60) → **197 bytes**
- 12-frame sequence (triangle + centre dot) → **474 bytes**
- rect + circle + polygon converted from SVG → **75 bytes**, rendered exactly as specified on emery

Then render it: a PDC that the firmware dislikes draws nothing (no error), so the only reliable check is a screenshot of a running app.

## Gotchas

- PDC files cannot be created at runtime — always ship them as `raw` resources.
- The SDK does **not** convert SVG at build time: the `.pdc` is the resource.
- Illustrator/Inkscape exports often contain unsupported elements (`text`, `image`, `use`, gradients) — `svg2pdc` reports them and skips; convert text to paths first.
- Keep point counts low: vectors are cheap to store but each point costs render time on the watch CPU.
- `play_count` is metadata for `gdraw_command_sequence_get_play_count()`; the frames still must be stepped by your own timer or by `get_frame_by_elapsed()`.

Source: <https://developer.repebble.com/tutorials/advanced/vector-animations/>
