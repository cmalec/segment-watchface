#!/usr/bin/env python3
"""Check an appstore/ tree against what the portal expects before submitting.

Verifies the two icons, one 720x320 banner per platform, and each platform's
screenshots at that platform's own screen size (1..5 of them). Reads the
description out of listing.md when it is there and checks the 1600 character
limit. Exits non-zero if anything is off.

    python3 audit_assets.py appstore
    python3 audit_assets.py appstore --platforms aplite,emery
"""
import argparse
import glob
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emulator import PLATFORMS  # noqa: E402

DESC_LIMIT = 1600
MAX_SHOTS = 5


def png_size(path):
    """Read width and height from the IHDR chunk."""
    with open(path, "rb") as f:
        header = f.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    return struct.unpack(">II", header[16:24])


def check(label, path, want, problems):
    got = png_size(path)
    if got is None:
        problems.append(f"{label}: {path} is not a PNG")
    elif got != want:
        problems.append(f"{label}: {path} is {got[0]}x{got[1]}, want {want[0]}x{want[1]}")


def audit(root, platforms):
    problems = []
    for name, want in (("icon-144.png", (144, 144)), ("icon-48.png", (48, 48))):
        path = os.path.join(root, name)
        if os.path.exists(path):
            check("icon", path, want, problems)
        else:
            problems.append(f"icon: {path} is missing")

    for platform in platforms:
        size = PLATFORMS[platform]
        banner = os.path.join(root, "banner", f"{platform}.png")
        if os.path.exists(banner):
            check("banner", banner, (720, 320), problems)
        else:
            problems.append(f"banner: {banner} is missing")

        shots = sorted(glob.glob(os.path.join(root, "screenshots", f"{platform}-*.png")))
        if not shots:
            problems.append(f"screenshots: none for {platform}")
        elif len(shots) > MAX_SHOTS:
            problems.append(f"screenshots: {len(shots)} for {platform}, "
                            f"max {MAX_SHOTS}")
        for path in shots:
            check(f"screenshot ({platform})", path, size, problems)

    listing = os.path.join(root, "listing.md")
    if os.path.exists(listing):
        block = re.search(r"```text\n(.*?)```", open(listing).read(), re.S)
        if block is None:
            problems.append("listing: no ```text description block in listing.md")
        elif len(block.group(1)) > DESC_LIMIT:
            problems.append(f"listing: description is {len(block.group(1))} "
                            f"chars, limit {DESC_LIMIT}")

    for platform in platforms:
        n = len(glob.glob(os.path.join(root, "screenshots", f"{platform}-*.png")))
        print(f"  {platform:<8} {PLATFORMS[platform][0]}x{PLATFORMS[platform][1]}  "
              f"{n} screenshot(s)")
    if problems:
        print("\nFAIL:")
        for problem in problems:
            print(f"  - {problem}")
        return 1
    print("\nPASS - every asset is the size the portal asks for")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("root", nargs="?", default="appstore")
    parser.add_argument("--platforms", default=",".join(PLATFORMS))
    args = parser.parse_args()
    sys.exit(audit(args.root, args.platforms.split(",")))


if __name__ == "__main__":
    main()
