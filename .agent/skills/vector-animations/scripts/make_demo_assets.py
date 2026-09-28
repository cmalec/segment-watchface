#!/usr/bin/env python3
"""Generate the demo PDC assets used by the watchface template.

Usage:
    python3 make_demo_assets.py <project_dir>

Writes <project_dir>/resources/pdcs/sun.pdc (image) and spinner.pdc (sequence).
Both are plain geometry generated with make_pdc.py — no SVG tooling required.
"""

import argparse
import math
import pathlib
import sys

sys.dont_write_bytecode = True          # keep the skill directory free of __pycache__
sys.path.insert(0, str(pathlib.Path(__file__).parent))

from make_pdc import argb8, circle, image, path, sequence  # noqa: E402

SIZE = 60
AMBER = argb8(255, 170, 0)
CYAN = argb8(0, 170, 255)
WHITE = argb8(255, 255, 255)


def sun_image():
    """A filled disc with eight rays — a static vector icon."""
    centre = (SIZE / 2, SIZE / 2)
    commands = [circle(centre, 9, fill=AMBER)]

    for i in range(8):
        angle = math.radians(i * 45)
        cos_a, sin_a = math.cos(angle), math.sin(angle)

        def rotate(x, y):
            return (centre[0] + x * cos_a - y * sin_a, centre[1] + x * sin_a + y * cos_a)

        commands.append(path([rotate(-3, -13), rotate(3, -13), rotate(0, -22)], fill=AMBER))
    return image(SIZE, SIZE, commands)


def spinner_sequence(frames=12, radius=16):
    """A triangle rotating in place — one PDC sequence, `frames` frames."""
    frame_list = []
    centre = (SIZE / 2, SIZE / 2)

    for index in range(frames):
        angle = 2 * math.pi * index / frames
        points = []
        for corner in range(3):
            corner_angle = angle + corner * 2 * math.pi / 3
            points.append((centre[0] + radius * math.sin(corner_angle),
                           centre[1] - radius * math.cos(corner_angle)))
        frame_list.append([path(points, fill=CYAN, stroke=WHITE, width=1),
                           circle(centre, 3, fill=WHITE)])

    return sequence(SIZE, SIZE, frame_list, duration=50, play_count=1)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("project", nargs="?", default=".", help="project root (default: .)")
    args = ap.parse_args()

    root = pathlib.Path(args.project).resolve()
    if not (root / "package.json").is_file():
        print(f"error: no package.json in {root} — pass the project root", file=sys.stderr)
        return 1

    out_dir = root / "resources/pdcs"
    out_dir.mkdir(parents=True, exist_ok=True)

    for name, data in (("sun.pdc", sun_image()), ("spinner.pdc", spinner_sequence())):
        (out_dir / name).write_bytes(data)
        print(f"ok: wrote {out_dir / name} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
