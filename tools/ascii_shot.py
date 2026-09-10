#!/usr/bin/env python3
"""Render a region of an emulator screenshot as ASCII art (dev debug helper).

Usage: python3 tools/ascii_shot.py SHOT.png [full|top|bottom]
  full    whole frame, 3x3 downsample
  top     status strip y50-86, 2x2 downsample
  bottom  bottom strip y200-228, 2x2 downsample
"""
import sys
from PIL import Image

REGIONS = {
    'full':   (0, None, 3),
    'top':    (50, 86, 2),
    'bottom': (200, 228, 2),
}

def main():
    if len(sys.argv) != 3 or sys.argv[2] not in REGIONS:
        sys.exit(__doc__)
    path, region = sys.argv[1], sys.argv[2]
    y0, y1, step = REGIONS[region]
    im = Image.open(path).convert('L')
    w, h = im.size
    px = im.load()
    if y1 is None:
        y1 = h
    for y in range(y0, y1, step):
        row = ''
        for x in range(0, w, step):
            vals = [px[x + dx, y + dy]
                    for dx in range(step) if x + dx < w
                    for dy in range(step) if y + dy < h]
            avg = sum(vals) / len(vals)
            row += '#' if avg > 128 else ('+' if avg > 60 else '.')
        if region == 'full':
            print(row)
        else:
            print(f'{y:3d} {row}')

if __name__ == '__main__':
    main()
