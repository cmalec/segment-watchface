#!/usr/bin/env python3
"""Render appstore icons (144x144, 48x48) and one 720x320 banner per platform
from SVG, each banner framing that platform's own screenshot in a drawn
device bezel - the listing screenshots themselves stay unframed.

Needs rsvg-convert (librsvg), e.g. `brew install librsvg`. Replace MARK_SVG and
the CLI copy with your own; everything else (geometry, frames, layout) is the
reusable part.

    python3 make_assets.py --screens appstore/screenshots --out appstore \
        --title myapp --tagline "What it does, in a line" \
        --cue "5% and 10% rows" --cue "offline"

Screenshots are matched per platform as <platform>-results*.png, falling back
to <platform>-*.png.
"""
import argparse
import base64
import glob
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emulator import PLATFORMS  # noqa: E402

ROUND = ("chalk", "gabbro")
BANNER = (720, 320)
BEZEL = 18

# Replace with the app's own mark: any SVG drawn in a 144x144 space, centred
# on (72, 72). Keep it readable at 48x48 - few shapes, high contrast.
MARK_SVG = """
  <circle cx="72" cy="72" r="34" fill="none" stroke="#e8eaed" stroke-width="9"/>
  <rect x="30" y="66" width="84" height="12" rx="6" fill="#e8eaed"/>
"""


def render(svg, path, width, height):
    tmp = f"{path}.svg"
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(tmp, "w") as f:
        f.write(svg)
    subprocess.run(["rsvg-convert", "-w", str(width), "-h", str(height),
                    "-o", path, tmp], check=True)
    os.remove(tmp)
    print(f"wrote {path} ({width}x{height})")


def icon_svg():
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="144" height="144"
     viewBox="0 0 144 144">
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#242b35"/><stop offset="1" stop-color="#0b0e12"/>
    </linearGradient>
  </defs>
  <rect width="144" height="144" rx="30" fill="url(#bg)"/>
  <rect x="1" y="1" width="142" height="142" rx="29" fill="none"
        stroke="#ffffff1f" stroke-width="2"/>
  {MARK_SVG}
</svg>
'''


def banner_svg(platform, screenshot, title, tagline, cues):
    width, height = PLATFORMS[platform]
    scale = 250 / height
    sw, sh = width * scale, height * scale
    bx, by = 664 - sw - BEZEL * 2, (BANNER[1] - sh - BEZEL * 2) / 2
    cx, cy = bx + BEZEL + sw / 2, by + BEZEL + sh / 2
    radius = min(sw, sh) / 2
    shot = base64.b64encode(open(screenshot, "rb").read()).decode()

    if platform in ROUND:
        body = (f'<circle cx="{cx}" cy="{cy}" r="{radius + BEZEL}" '
                f'fill="#2b313a" stroke="#454d58" stroke-width="2"/>')
        shadow = (f'<circle cx="{cx + 8}" cy="{cy + 10}" r="{radius + BEZEL}" '
                  f'fill="#000000" opacity="0.45"/>')
        screen = (f'<circle cx="{cx}" cy="{cy}" r="{radius}" fill="#000"/>'
                  f'<clipPath id="screen"><circle cx="{cx}" cy="{cy}" '
                  f'r="{radius}"/></clipPath>')
    else:
        body = (f'<rect x="{bx}" y="{by}" width="{sw + BEZEL * 2}" '
                f'height="{sh + BEZEL * 2}" rx="{32 + BEZEL}" fill="#2b313a" '
                f'stroke="#454d58" stroke-width="2"/>')
        shadow = (f'<rect x="{bx + 8}" y="{by + 10}" width="{sw + BEZEL * 2}" '
                  f'height="{sh + BEZEL * 2}" rx="{32 + BEZEL}" '
                  f'fill="#000000" opacity="0.45"/>')
        screen = (f'<rect x="{bx + BEZEL}" y="{by + BEZEL}" width="{sw}" '
                  f'height="{sh}" rx="28" fill="#000"/>'
                  f'<clipPath id="screen"><rect x="{bx + BEZEL}" '
                  f'y="{by + BEZEL}" width="{sw}" height="{sh}" rx="28"/></clipPath>')

    # Chips are sized from their text (Helvetica at 15px is ~7.4px per
    # character) and kept inside the copy column, x 52..366.
    chips, x = [], 52
    for cue in cues:
        w = 30 + len(cue) * 7.4
        chips.append(f'<rect x="{x:.0f}" y="236" width="{w:.0f}" height="34" '
                     f'rx="17" fill="#ffffff14" stroke="#ffffff3d"/>'
                     f'<text x="{x + 15:.0f}" y="258" font-size="15" '
                     f'font-family="Helvetica, Arial, sans-serif" '
                     f'fill="#dfe5ec">{cue}</text>')
        x += w + 12

    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{BANNER[0]}"
     height="{BANNER[1]}" viewBox="0 0 {BANNER[0]} {BANNER[1]}">
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0" stop-color="#161b22"/><stop offset="1" stop-color="#0a0c10"/>
    </linearGradient>
  </defs>
  <rect width="{BANNER[0]}" height="{BANNER[1]}" fill="url(#bg)"/>
  <text x="52" y="122" font-family="Helvetica, Arial, sans-serif" font-size="74"
        font-weight="bold" fill="#ffffff">{title}</text>
  <text x="54" y="168" font-family="Helvetica, Arial, sans-serif" font-size="24"
        fill="#aab2bd">{tagline}</text>
  {''.join(chips)}
  {shadow}
  {body}
  {screen}
  <g clip-path="url(#screen)">
    <image x="{bx + BEZEL}" y="{by + BEZEL}" width="{sw}" height="{sh}"
           href="data:image/png;base64,{shot}"/>
  </g>
</svg>
'''


def hero_shot(screens, platform):
    for pattern in (f"{platform}-results*.png", f"{platform}-*.png"):
        matches = sorted(glob.glob(os.path.join(screens, pattern)))
        if matches:
            return matches[0]
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--screens", default="appstore/screenshots")
    parser.add_argument("--out", default="appstore")
    parser.add_argument("--title", required=True)
    parser.add_argument("--tagline", required=True)
    parser.add_argument("--cue", action="append", default=[])
    parser.add_argument("--platforms", default=",".join(PLATFORMS))
    args = parser.parse_args()

    render(icon_svg(), os.path.join(args.out, "icon-144.png"), 144, 144)
    render(icon_svg(), os.path.join(args.out, "icon-48.png"), 48, 48)
    missing = []
    for platform in args.platforms.split(","):
        shot = hero_shot(args.screens, platform)
        if shot is None:
            missing.append(platform)
            continue
        render(banner_svg(platform, shot, args.title, args.tagline, args.cue),
               os.path.join(args.out, "banner", f"{platform}.png"), *BANNER)
    if missing:
        print(f"no screenshot for: {', '.join(missing)}", file=sys.stderr)


if __name__ == "__main__":
    main()
