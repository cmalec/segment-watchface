#!/usr/bin/env python3
"""Generate Pebble Draw Command (PDC) vector images and sequences.

Dependency-free alternative to the upstream `svg2pdc.py` (which is Python 2 and
needs `svg.path`): describe shapes directly, or drive it from a JSON spec.

    # as a library
    from make_pdc import image, sequence, path, circle, argb8
    open("sun.pdc", "wb").write(image(30, 30, [circle((15, 15), 8, fill=argb8(255, 170, 0))]))

    # from a JSON spec (see the module docstring in SPEC below)
    python3 make_pdc.py spec.json -o resources/pdcs/spinner.pdc

File format (little-endian), matching Pebble firmware and svg2pdc output:

    file   = magic[4] + uint32 payload_size + payload
    magic  = b"PDCI" (image) | b"PDCS" (sequence)
    header = uint8 version(1) + uint8 reserved(0) + int16 width + int16 height
    image  = header + command_list
    seque  = header + uint16 play_count + uint16 frame_count + frames
    frame  = uint16 duration_ms + command_list
    cmds   = uint16 command_count + command*
    cmd    = uint8 type + uint8 reserved(0) + uint8 stroke_color + uint8 stroke_width
             + uint8 fill_color
             + path:   uint8 open + uint8 unused
             + circle: uint16 radius
             + uint16 point_count + point*      (point = int16 x + int16 y)
    type   = 1 path | 2 circle | 3 precise path (coordinates in 1/8 px units)

Colors are Pebble 8-bit ARGB2222 (2 bits per channel); `argb8()` encodes them.
"""

import argparse
import json
import pathlib
import struct
import sys

PDC_IMAGE_MAGIC = b"PDCI"
PDC_SEQUENCE_MAGIC = b"PDCS"

DRAW_COMMAND_VERSION = 1
TYPE_PATH = 1
TYPE_CIRCLE = 2
TYPE_PRECISE_PATH = 3

DEFAULT_FRAME_DURATION = 33   # ms — ~30 fps
DEFAULT_PLAY_COUNT = 1


def argb8(r, g, b, a=255):
    """Pack RGB(A) 0-255 into Pebble's 8-bit ARGB2222 color byte."""
    return ((a >> 6) << 6) | ((r >> 6) << 4) | ((g >> 6) << 2) | (b >> 6)


def path(points, fill=0, stroke=0, width=0, open=False, precise=False):
    """Polyline/polygon. `points` are (x, y) in pixels (Pebble coordinate system)."""
    return {
        "type": TYPE_PRECISE_PATH if precise else TYPE_PATH,
        "points": [(x, y) for x, y in points],
        "fill": fill,
        "stroke": stroke,
        "width": width,
        "open": open,
    }


def circle(center, radius, fill=0, stroke=0, width=0):
    """Circle; the point list holds the centre so shapes can be moved at runtime."""
    return {
        "type": TYPE_CIRCLE,
        "points": [tuple(center)],
        "radius": radius,
        "fill": fill,
        "stroke": stroke,
        "width": width,
    }


def _command_bytes(command):
    kind = command["type"]
    out = struct.pack("<BBBB", kind, 0, command["stroke"], command["width"])
    out += struct.pack("<B", command["fill"])

    if kind == TYPE_CIRCLE:
        out += struct.pack("<H", int(command["radius"]))
        points = command["points"]
    else:
        out += struct.pack("<BB", 1 if command.get("open") else 0, 0)
        scale = 8 if kind == TYPE_PRECISE_PATH else 1
        points = [(int(round(x * scale)), int(round(y * scale))) for x, y in command["points"]]

    out += struct.pack("<H", len(points))
    for x, y in points:
        out += struct.pack("<hh", int(x), int(y))
    return out


def _command_list_bytes(commands):
    out = struct.pack("<H", len(commands))
    for command in commands:
        out += _command_bytes(command)
    return out


def _header_bytes(width, height):
    return struct.pack("<BBhh", DRAW_COMMAND_VERSION, 0, int(width), int(height))


def image(width, height, commands):
    """Serialise a PDC image. `commands` is a list built with path()/circle()."""
    payload = _header_bytes(width, height) + _command_list_bytes(commands)
    return PDC_IMAGE_MAGIC + struct.pack("<I", len(payload)) + payload


def sequence(width, height, frames, duration=DEFAULT_FRAME_DURATION, play_count=DEFAULT_PLAY_COUNT):
    """Serialise a PDC sequence.

    `frames` is a list of command lists; `duration` (ms) applies to every frame
    unless a frame is given as {"duration": ms, "commands": [...]}.
    """
    payload = _header_bytes(width, height) + struct.pack("<HH", int(play_count), len(frames))
    for frame in frames:
        if isinstance(frame, dict):
            frame_commands = frame["commands"]
            frame_duration = frame.get("duration", duration)
        else:
            frame_commands = frame
            frame_duration = duration
        payload += struct.pack("<H", int(frame_duration)) + _command_list_bytes(frame_commands)
    return PDC_SEQUENCE_MAGIC + struct.pack("<I", len(payload)) + payload


# ---------------------------------------------------------------- JSON spec

SPEC = """\
{
  "type": "sequence",            // or "image"
  "width": 60, "height": 60,
  "duration": 33, "play_count": 1,        // sequence only
  "commands": [                            // for "image"
    {"shape": "circle", "center": [30, 30], "radius": 12, "fill": [255, 170, 0]}
  ],
  "frames": [                              // for "sequence"
    {"commands": [{"shape": "path", "points": [[30, 12], [36, 30], [30, 48]],
                   "fill": [0, 170, 255], "stroke": [255, 255, 255], "width": 2}]}
  ]
}
Colors are [r, g, b] or [r, g, b, a]; omitted colours are transparent (0).
"""


def _color(spec_value):
    if spec_value is None:
        return 0
    return argb8(*spec_value) if isinstance(spec_value, list) else int(spec_value)


def command_from_spec(spec):
    shape = spec.get("shape", "path")
    if shape == "circle":
        return circle(spec["center"], spec["radius"], fill=_color(spec.get("fill")),
                      stroke=_color(spec.get("stroke")), width=spec.get("width", 0))
    if shape == "path":
        return path([tuple(p) for p in spec["points"]], fill=_color(spec.get("fill")),
                    stroke=_color(spec.get("stroke")), width=spec.get("width", 0),
                    open=spec.get("open", False), precise=spec.get("precise", False))
    raise ValueError(f"unknown shape: {shape}")


def build_from_spec(spec):
    width, height = spec["width"], spec["height"]
    if spec.get("type", "image") == "sequence":
        frames = [{"duration": f.get("duration", spec.get("duration", DEFAULT_FRAME_DURATION)),
                   "commands": [command_from_spec(c) for c in f["commands"]]}
                  for f in spec["frames"]]
        return sequence(width, height, frames, duration=spec.get("duration", DEFAULT_FRAME_DURATION),
                        play_count=spec.get("play_count", DEFAULT_PLAY_COUNT))
    return image(width, height, [command_from_spec(c) for c in spec["commands"]])


def main():
    ap = argparse.ArgumentParser(description="Generate a PDC image or sequence from a JSON spec")
    ap.add_argument("spec", help="JSON spec file")
    ap.add_argument("-o", "--output", required=True, help="output .pdc path")
    args = ap.parse_args()

    spec = json.loads(pathlib.Path(args.spec).read_text())
    data = build_from_spec(spec)
    out = pathlib.Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(data)
    print(f"ok: wrote {out} ({len(data)} bytes, {spec.get('type', 'image')})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
