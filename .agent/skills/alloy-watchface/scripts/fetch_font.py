#!/usr/bin/env python3
"""Fetch the Jersey 10 font into an Alloy project's asset directory.

Usage:
    python3 fetch_font.py [project_dir] [--force]

Downloads Jersey10-Regular.ttf (OFL-1.1, Google Fonts) to
<project_dir>/src/embeddedjs/assets/Jersey10-Regular.ttf. The font is required
by the `*-alpha` resources declared in src/embeddedjs/manifest.json; any other
TTF works too — update the manifest `source` if you swap fonts.

Manual fallback: download https://fonts.google.com/specimen/Jersey+10 and save
it as <project>/src/embeddedjs/assets/Jersey10-Regular.ttf
"""

import argparse
import hashlib
import pathlib
import sys
import urllib.request

FONT = "Jersey10-Regular.ttf"

# Known OFL-1.1 releases of the font, by sha256.
KNOWN = {
    "d661892ab865793dbbf1d4d9514ca2d3c5adb5b1f06ad302fff4d021042dd82a": "coredevices tutorial copy",
    "db9cbd091617048a145d249daa2b815fe7083be6ab66ac26626e21a4e01c3e82": "Google Fonts, 2026-09",
}

URLS = (
    "https://raw.githubusercontent.com/google/fonts/main/ofl/jersey10/Jersey10-Regular.ttf",
    "https://raw.githubusercontent.com/coredevices/alloy-watchface-tutorial/main/part6/src/embeddedjs/assets/Jersey10-Regular.ttf",
)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("project", nargs="?", default=".", help="project root (default: .)")
    ap.add_argument("--force", action="store_true", help="re-download even if the file exists")
    args = ap.parse_args()

    root = pathlib.Path(args.project).resolve()
    if not (root / "package.json").is_file():
        print(f"error: no package.json in {root} — pass the Alloy project root", file=sys.stderr)
        return 1

    dest = root / "src/embeddedjs/assets" / FONT
    dest.parent.mkdir(parents=True, exist_ok=True)

    if dest.is_file() and not args.force:
        digest = hashlib.sha256(dest.read_bytes()).hexdigest()
        if digest in KNOWN:
            print(f"ok: {dest} already present ({KNOWN[digest]})")
            return 0
        print(f"warn: {dest} exists but sha256 differs — re-downloading", file=sys.stderr)

    errors = []
    for url in URLS:
        try:
            print(f"downloading {url}")
            with urllib.request.urlopen(url, timeout=60) as response:
                data = response.read()
        except OSError as exc:  # network / HTTP errors
            errors.append(f"{url}: {exc}")
            continue

        if data[:4] != b"\x00\x01\x00\x00":  # TrueType sfnt magic
            errors.append(f"{url}: not a TrueType font")
            continue

        digest = hashlib.sha256(data).hexdigest()
        if digest in KNOWN:
            print(f"sha256 verified ({KNOWN[digest]})")
        else:
            print(f"note: unrecognized sha256 {digest} — is this really Jersey 10 (OFL-1.1)?")

        dest.write_bytes(data)
        print(f"ok: wrote {len(data)} bytes to {dest}")
        return 0

    print("error: could not fetch the font:", file=sys.stderr)
    for line in errors:
        print(f"  - {line}", file=sys.stderr)
    print(f"\nDownload it manually from https://fonts.google.com/specimen/Jersey+10 "
          f"and save it as {dest}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
