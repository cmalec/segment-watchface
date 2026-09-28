#!/usr/bin/env python3
"""Check that every weather-row icon the watch draws matches the font resource.

The icon is a glyph of the weather-icons font (see .agent/skills/weather-icons).
Two things can go wrong silently and this tool exists to catch them:

  * the glyph is missing from the baked font (a stale `characterRegex` cache, or
    a codepoint the .ttf does not carry) — the watch then draws the font's
    wildcard glyph instead;
  * the icon's text layer is narrower than the glyph's advance, and Pebble's
    text layout draws a *truncated* layer as an ellipsis ("...").

So: decode the glyph bitmaps out of the built font resource, drive the face on
the emulator with one AppMessage per condition, and compare the ink the watch
painted with the bitmap that should have gone there.

Usage (build and install the face first — the tool reads the built resource):
    tools/verify_weather_glyphs.py                 # both phases, current hour decides
    tools/verify_weather_glyphs.py --phase day     # assert the sun forms
    tools/verify_weather_glyphs.py --keep          # keep the screenshots

Plain python3: the font resource is decoded here, and so is the screenshot PNG
(the repo's other tools want Pillow, which this one avoids).
"""

import argparse
import glob
import pickle
import re
import struct
import subprocess
import sys
import time
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# WMO code -> condition, mirroring weather_cond_from_wmo() in src/c/helpers.c.
# Keep in step with that function (the C side is the one the watch runs, and
# tests/host/test_helpers.c guards it).
WMO_CODES = {
    0: 'CLEAR', 2: 'PARTLY_CLOUDY', 3: 'CLOUDY', 45: 'FOG', 51: 'DRIZZLE', 61: 'RAIN',
    80: 'SHOWERS', 66: 'SLEET', 71: 'SNOW', 95: 'THUNDERSTORM',
}
# Conditions drawn with the same glyph day and night: half the table is enough.
PHASES = ('day', 'night')


# --------------------------------------------------------------- source facts

def read_glyph_table():
    """Parse the condition -> {day, night} glyph table out of decorations.c."""
    src = (ROOT / 'src/c/decorations.c').read_text()
    table = {}
    for cond, day, night in re.findall(
            r'\[WEATHER_COND_(\w+)\]\s*=\s*\{\s*(NULL|"(?:\\x[0-9A-Fa-f]{2})+")'
            r'\s*,\s*(NULL|"(?:\\x[0-9A-Fa-f]{2})+")', src):
        def decode(literal):
            if literal == 'NULL':
                return None
            return bytes(int(b, 16) for b in re.findall(r'\\x([0-9A-Fa-f]{2})', literal))
        table[cond] = {'day': decode(day), 'night': decode(night)}
    if not table:
        sys.exit('could not read the glyph table from src/c/decorations.c')
    return table


def read_night_hours():
    src = (ROOT / 'src/c/decorations.c').read_text()
    start = int(re.search(r'#define WEATHER_NIGHT_START (\d+)', src).group(1))
    end = int(re.search(r'#define WEATHER_NIGHT_END (\d+)', src).group(1))
    return start, end


def read_icon_rect():
    src = (ROOT / 'src/c/_globals.h').read_text()
    x, y, w, h = map(int, re.search(r'#define DECORATIONS_WEATHER_ICON GRect\(([\d, ]+)\)', src).group(1).split(','))
    return x, y, w, h


def read_message_key(name):
    """Message key ids are generated into build/src/message_keys.auto.c."""
    src = (ROOT / 'build/src/message_keys.auto.c').read_text()
    return int(re.search(rf'uint32_t MESSAGE_KEY_{name} = (\d+);', src).group(1))


# ------------------------------------------------------------- font resource

class _Any:
    def __setstate__(self, state):
        self.__dict__.update(state)


class _Shim(pickle.Unpickler):
    """Unpickle the SDK's ResourceObject without importing the SDK."""

    def find_class(self, module, name):
        return _Any


def baked_glyphs(platform, resource_name):
    """codepoint -> (width, height, rows of bits) for the built font resource."""
    pattern = str(ROOT / f'build/{platform}/resources/**/*.{resource_name}.reso')
    hits = glob.glob(pattern, recursive=True)
    if not hits:
        sys.exit(f'no built font resource {resource_name} — run `pebble build` first\n(looked for {pattern})')
    data = _Shim(open(hits[0], 'rb')).load().data

    # Font v3, see the SDK's tools/font/fontgen.py: info, hash table, offset
    # tables, then the glyph table the offsets point into.
    version, max_height, num_glyphs, wildcard, hash_size, cp_bytes, info_size, features = \
        struct.unpack_from('<BBHHBBBB', data, 0)
    if version != 3:
        sys.exit(f'unsupported font resource version {version}')
    pos = info_size
    hash_table = data[pos:pos + hash_size * 4]
    pos += hash_size * 4
    off_fmt = 'H' if features & 0x01 else 'I'
    record = struct.calcsize('<H' + off_fmt)
    found, offset_region = {}, 0
    for i in range(hash_size):
        _hash, bucket_size, offset = struct.unpack_from('<BBH', hash_table, i * 4)
        if not bucket_size:
            continue
        offset_region = max(offset_region, offset + bucket_size * record)
        for j in range(bucket_size):
            codepoint, glyph_offset = struct.unpack_from('<H' + off_fmt, data, pos + offset + j * record)
            found[codepoint] = glyph_offset
    glyph_table = data[pos + offset_region:]

    glyphs = {}
    for codepoint, offset in found.items():
        width, height, _left, _bottom, _advance = struct.unpack_from('<BBbbb', glyph_table, offset)
        bits = []
        for k in range(0, ((width * height + 31) // 32) * 4, 4):
            word = struct.unpack_from('<I', glyph_table, offset + 5 + k)[0]
            bits.extend((word >> i) & 1 for i in range(32))
        glyphs[codepoint] = (width, height, [[bits[y * width + x] for x in range(width)] for y in range(height)])
    return glyphs, wildcard


# --------------------------------------------------------------------- screen

def load_png(path):
    """Minimal PNG reader: (width, height, grayscale bytes). 8-bit, non-interlaced."""
    raw = Path(path).read_bytes()
    if raw[:8] != b'\x89PNG\r\n\x1a\n':
        sys.exit(f'{path}: not a PNG')
    pos, idat, width = 8, b'', None
    while pos < len(raw):
        length, kind = struct.unpack('>I4s', raw[pos:pos + 8])
        chunk = raw[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b'IHDR':
            width, height, depth, color, _comp, _filter, interlace = struct.unpack('>IIBBBBB', chunk)
            assert depth == 8 and interlace == 0, 'only 8-bit non-interlaced PNGs'
        elif kind == b'IDAT':
            idat += chunk
        elif kind == b'IEND':
            break
    channels = {0: 1, 2: 3, 4: 2, 6: 4}[color]
    stride = width * channels
    out = bytearray(width * height)
    previous = bytearray(stride)
    stream = zlib.decompress(idat)
    pos = 0
    for y in range(height):
        filter_type = stream[pos]
        pos += 1
        line = bytearray(stream[pos:pos + stride])
        pos += stride
        if filter_type == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif filter_type == 2:
            for i in range(stride):
                line[i] = (line[i] + previous[i]) & 0xFF
        elif filter_type == 3:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + previous[i]) >> 1)) & 0xFF
        elif filter_type == 4:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                up = previous[i]
                corner = previous[i - channels] if i >= channels else 0
                estimate = left + up - corner
                da, db, dc = abs(estimate - left), abs(estimate - up), abs(estimate - corner)
                nearest = left if (da <= db and da <= dc) else (up if db <= dc else corner)
                line[i] = (line[i] + nearest) & 0xFF
        for x in range(width):
            i = x * channels
            out[y * width + x] = line[i] if channels < 3 else (line[i] * 299 + line[i + 1] * 587 + line[i + 2] * 114) // 1000
        previous = line
    return width, height, out


def screen_ink(path, rect):
    """The icon band's ink, cropped to its bounding box (None if nothing drew)."""
    x, y, w, h = rect
    width, _height, gray = load_png(path)
    points = [(px, py) for py in range(y, y + h) for px in range(x, x + w)
              if gray[py * width + px] < 128]
    if not points:
        return None
    x0, x1 = min(p[0] for p in points), max(p[0] for p in points)
    y0, y1 = min(p[1] for p in points), max(p[1] for p in points)
    return [[1 if gray[py * width + px] < 128 else 0 for px in range(x0, x1 + 1)]
            for py in range(y0, y1 + 1)]


def glyph_ink(glyph):
    width, height, rows = glyph
    points = [(x, y) for y in range(height) for x in range(width) if rows[y][x]]
    x0, x1 = min(p[0] for p in points), max(p[0] for p in points)
    y0, y1 = min(p[1] for p in points), max(p[1] for p in points)
    return [[rows[y][x] for x in range(x0, x1 + 1)] for y in range(y0, y1 + 1)]


def compare(got, expected):
    """(matched, note). The icon band deliberately clips the top row of the one
    glyph that reaches above the font's line box (day-cloudy); that shows up as
    the expected ink minus its first row and is reported, not failed."""
    if got is None or expected is None:
        return False, 'nothing drawn'
    if got == expected:
        return True, '100% match'
    if len(expected) > 1 and got == expected[1:]:
        return True, f'clip: top row ({_ink_count(expected[0])} px) is cut'
    if (len(got), len(got[0])) != (len(expected), len(expected[0])):
        return False, f'{len(got[0])}x{len(got)} vs {len(expected[0])}x{len(expected)}'
    same = sum(1 for y in range(len(got)) for x in range(len(got[0])) if got[y][x] == expected[y][x])
    total = len(got) * len(got[0])
    return False, f'{100 * same / total:.1f}% match'


def _ink_count(row):
    return sum(1 for value in row if value)


# ----------------------------------------------------------------------- main

def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--emulator', default='emery')
    parser.add_argument('--phase', choices=PHASES, help='assert which form the face should draw')
    parser.add_argument('--keep', action='store_true', help='keep the screenshots (default: /tmp)')
    args = parser.parse_args()

    night_start, night_end = read_night_hours()
    hour = time.localtime().tm_hour
    night = hour >= night_start or hour < night_end
    phase = args.phase or ('night' if night else 'day')
    if args.phase and args.phase != ('night' if night else 'day'):
        sys.exit(f'--phase {args.phase}, but the hour ({hour}:00) maps to '
                 f'{"night" if night else "day"} with WEATHER_NIGHT_START={night_start}/'
                 f'WEATHER_NIGHT_END={night_end}. Adjust the hours or the system clock.')

    table = read_glyph_table()
    rect = read_icon_rect()
    condition_key = read_message_key('wcond')
    now_key = read_message_key('wtemp_now')
    hi_key = read_message_key('wtemp_hi')
    lo_key = read_message_key('wtemp_lo')

    resource = re.search(r'"name": "(WX_ICON_\d+)"', (ROOT / 'package.json').read_text()).group(1)
    glyphs, wildcard = baked_glyphs(args.emulator, resource)
    print(f'{resource}: {len(glyphs)} baked glyphs (wildcard U+{wildcard:04X}); '
          f'icon band {rect}; verifying the {phase} forms\n')

    failures = 0
    for code, condition in WMO_CODES.items():
        literal = table.get(condition, {}).get(phase)
        if literal is None:
            print(f'SKIP  {condition:16} no {phase} glyph in the table')
            continue
        # three-byte UTF-8 (the icon font lives in the U+Exxx..U+Fxxx private-use area)
        codepoint = (literal[0] & 0x0F) << 12 | (literal[1] & 0x3F) << 6 | (literal[2] & 0x3F)

        subprocess.run(['pebble', 'send-app-message', '--emulator', args.emulator, '--uint',
                        f'{condition_key}={code}', f'{now_key}=19', f'{hi_key}=28', f'{lo_key}=12'],
                       check=False, capture_output=True)
        time.sleep(1.6)
        shot = f'/tmp/weather_glyph_{phase}_{condition.lower()}.png'
        subprocess.run(['pebble', 'screenshot', '--no-open', '--emulator', args.emulator, shot],
                       check=False, capture_output=True)

        if codepoint not in glyphs:
            print(f'FAIL  {condition:16} code {code:>2}  U+{codepoint:04X} not baked '
                  f'(regex/cache?) — the watch draws the wildcard')
            failures += 1
            continue
        ok, why = compare(screen_ink(shot, rect), glyph_ink(glyphs[codepoint]))
        print(f'{"PASS" if ok else "FAIL"}  {condition:16} code {code:>2}  U+{codepoint:04X}  {why}')
        if not ok:
            failures += 1
        if not args.keep and Path(shot).exists():
            Path(shot).unlink()

    print(f'\n{failures} failure(s)')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
