#!/usr/bin/env python3
"""Render Segment's appstore assets: the 144x144 / 48x48 icons and the 720x320
banner for each collection.

The layout, bezel and banner copy come from the pebble-appstore skill's renderer
(.agent/skills/pebble-appstore/scripts/make_assets.py) — this file only supplies
the app's own mark and CLI copy, which is the split that script asks for. Needs
rsvg-convert (librsvg).

    pebble build                                   # the banner embeds a screenshot
    .agent/skills/pebble-appstore/scripts/emulator.py emery appstore/screenshots/emery-1.png
    python3 tools/make_appstore_assets.py
    .agent/skills/pebble-appstore/scripts/audit_assets.py appstore --platforms emery
"""
import argparse
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKILL = os.path.join(ROOT, ".agent", "skills", "pebble-appstore", "scripts")
sys.path.insert(0, SKILL)
import make_assets  # noqa: E402  (the skill's renderer)

# Segment's mark, drawn in the skill's 144x144 space, centred on (72, 72): the
# face's own arrangement — white rounded panel, black seven-segment digits — and
# "7S" (for the seven-segment clock) in the segments themselves: 7 is a,b,c and S is
# the same pattern as 5, a,f,g,c,d. Bars carry the usual gaps, so the mark reads at
# 48x48 too.
BAR, W, H = 10, 40, 68          # thickness, digit box
PANEL = (10, 20, 124, 104)      # x, y, w, h of the panel


def _digit(x, y, segments, s_shape=False):
    """Seven-segment digit at (x, y): the classic bars, gapped, for `segments`.

        aaa
       f   b
       f   b
        ggg
       e   c
       e   c
        ddd
    """
    gap = 2
    mid = (H - BAR) // 2                        # top of the middle bar
    half = mid - BAR - gap                      # length of a vertical bar
    bars = {
        "a": (x, y, W, BAR),
        "f": (x, y + BAR + gap, BAR, half),
        "b": (x + W - BAR, y + BAR + gap, BAR, half),
        "g": (x, y + mid, W, BAR),
        "e": (x, y + mid + BAR + gap, BAR, half),
        "c": (x + W - BAR, y + mid + BAR + gap, BAR, half),
        "d": (x, y + H - BAR, W, BAR),
    }
    if s_shape:
        # A seven-segment display draws S and 5 with the same segments; the face's
        # own font separates them by trimming the top bar's ends and the bottom
        # bar's right, so the mark copies that.
        bars["a"] = (x + W // 4, y, W - W // 4 - 3, BAR)
        bars["d"] = (x, y + H - BAR, W - W // 4, BAR)
    return "".join(
        f'<rect x="{hx}" y="{hy}" width="{hw}" height="{hh}" rx="4" fill="#111318"/>'
        for hx, hy, hw, hh in (bars[seg] for seg in segments))


MARK_SVG = f'''
  <rect x="{PANEL[0]}" y="{PANEL[1]}" width="{PANEL[2]}" height="{PANEL[3]}" rx="16" fill="#f2f4f7"/>
  <rect x="{PANEL[0]}" y="{PANEL[1]}" width="{PANEL[2]}" height="{PANEL[3]}" rx="16"
        fill="none" stroke="#0b0e12" stroke-width="3"/>
  {_digit(22, 38, "abc")}         <!-- 7 -->
  {_digit(82, 38, "afgcd", s_shape=True)}  <!-- S -->
'''

make_assets.MARK_SVG = MARK_SVG


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--screens", default="appstore/screenshots")
    parser.add_argument("--out", default="appstore")
    parser.add_argument("--platforms", default="emery")
    parser.add_argument("--title", default="Segment")
    parser.add_argument("--tagline", default="Seven segment clock + weather & health")
    parser.add_argument("--cue", action="append", default=None,
                        help="banner chip; the footer defaults to a note instead")
    args = parser.parse_args()
    cues = args.cue or []
    note = "Built for the Time2. Inspired by 91 Dub"

    os.chdir(ROOT)
    # The skill's script owns the geometry; drive it with our mark and copy.
    sys.argv = ["make_assets.py", "--screens", args.screens, "--out", args.out,
                "--platforms", args.platforms, "--title", args.title,
                "--tagline", args.tagline]
    for cue in cues:
        sys.argv += ["--cue", cue]
    sys.argv += ["--note", note]
    make_assets.main()
    return 0


if __name__ == "__main__":
    sys.exit(main())
