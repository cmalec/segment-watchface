#!/usr/bin/env python3
"""Generate the Bluetooth disconnect icon used by the C watchface template.

Usage:
    python3 make_bt_icon.py <project_dir>

Writes <project_dir>/resources/images/bt-icon.png — a 30x30 white Bluetooth
rune on a transparent background (8-bit grayscale + alpha, no dependencies).
The watchface shows this bitmap in its BitmapLayer while the phone is
disconnected, so it is drawn with GCompOpSet over the face.

The generated PNG is committed in templates/watchface/resources/images/, so
this script only needed when you want to change the artwork.
"""

import argparse
import pathlib
import struct
import sys
import zlib

SIZE = 30
MARGIN = 3
STROKE_RADIUS = 1  # 1 -> 3 px wide strokes


def blank_canvas():
    return [[0] * SIZE for _ in range(SIZE)]


def stamp(pixels, x, y):
    for dy in range(-STROKE_RADIUS, STROKE_RADIUS + 1):
        for dx in range(-STROKE_RADIUS, STROKE_RADIUS + 1):
            xx, yy = x + dx, y + dy
            if 0 <= xx < SIZE and 0 <= yy < SIZE:
                pixels[yy][xx] = 255


def draw_line(pixels, x0, y0, x1, y1):
    """Bresenham with a square brush."""
    dx, dy = abs(x1 - x0), -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    while True:
        stamp(pixels, x0, y0)
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def write_png(path, pixels):
    """Minimal 8-bit grayscale+alpha PNG writer (two bytes per pixel: gray, alpha)."""
    rows = []
    for row in pixels:
        line = bytearray(b"\x00")  # filter 0 per scanline
        for alpha in row:
            line += bytes((255, alpha))  # white glyph, variable alpha
        rows.append(bytes(line))
    raw = b"".join(rows)

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", SIZE, SIZE, 8, 4, 0, 0, 0)  # gray + alpha
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", ihdr)
           + chunk(b"IDAT", zlib.compress(raw, 9))
           + chunk(b"IEND", b""))
    path.write_bytes(png)


def build_icon():
    """The Bluetooth bind rune: a spine plus two crossing chevrons."""
    pixels = blank_canvas()

    def px(x):
        return MARGIN + round(x * (SIZE - 2 * MARGIN))

    def py(y):
        return MARGIN + round(y * (SIZE - 2 * MARGIN))

    top, bottom = py(0.0), py(1.0)
    left, right = px(0.0), px(1.0)
    centre = px(0.5)

    draw_line(pixels, centre, top, centre, bottom)        # spine
    draw_line(pixels, centre, top, right, py(0.25))       # upper chevron out to the right
    draw_line(pixels, right, py(0.25), left, py(0.75))    #   leg crossing the spine
    draw_line(pixels, centre, bottom, right, py(0.75))    # lower chevron out to the right
    draw_line(pixels, right, py(0.75), left, py(0.25))    #   leg crossing the spine
    return pixels


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("project", nargs="?", default=".", help="project root (default: .)")
    args = ap.parse_args()

    root = pathlib.Path(args.project).resolve()
    if not (root / "package.json").is_file():
        print(f"error: no package.json in {root} — pass the project root", file=sys.stderr)
        return 1

    dest = root / "resources/images/bt-icon.png"
    dest.parent.mkdir(parents=True, exist_ok=True)
    write_png(dest, build_icon())
    print(f"ok: wrote {dest} ({dest.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
