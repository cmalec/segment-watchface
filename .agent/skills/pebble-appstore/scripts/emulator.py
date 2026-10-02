#!/usr/bin/env python3
"""Drive a Pebble emulator from Python: factory-fresh reset, launch, button
presses and device-resolution screenshots.

Standard library only (the pebble CLI does the work), so this drops into any
watchapp or watchface project. Import it from a capture script:

    import emulator as emu
    emu.reset("emery")                      # wipe + install twice
    emu.launch("emery")                     # back, back, select... until dark
    emu.button("emery", "select")           # up | down | select | back
    emu.hold("emery", "select")             # long press, e.g. to open a menu
    path = emu.shot("emery", "shots/menu.png")

Or run it directly for a smoke test:

    python3 emulator.py emery shots/smoke.png
"""
import os
import struct
import tempfile
import subprocess
import sys
import time
import zlib

# Screen size per platform, as the appstore stores screenshots.
PLATFORMS = {
    "aplite": (144, 168), "basalt": (144, 168), "chalk": (180, 180),
    "diorite": (144, 168), "emery": (200, 228), "flint": (144, 168),
    "gabbro": (260, 260),
}
# A mostly-dark frame is the app, not the launcher: the launcher is ~0.02 dark
# and a watchface with a big light panel is still ~0.4-0.5. Predominantly *light*
# apps need their own is_app= predicate - diff_ratio() against a reference frame
# is the tool for that (see the module docstring).
DARK = 0.4
PROBE_DIR = os.path.join(tempfile.gettempdir(), "pebble-emu-probe")


def pebble(emulator, *args, timeout=180):
    return subprocess.run(["pebble", *args, "--emulator", emulator],
                          capture_output=True, text=True, timeout=timeout)


def install(emulator):
    result = pebble(emulator, "install")
    if "succeeded" not in result.stdout + result.stderr:
        raise SystemExit(f"FAIL: install on {emulator}: "
                         f"{result.stdout}{result.stderr}")


def reset(emulator):
    """Factory-fresh emulator, so captures show the app's own defaults.

    `pebble wipe` only clears a *running* emulator, and the first install
    after a wipe leaves the app out of the launcher, hence the order:
    kill, install (starts it), wipe, kill, install, install.
    """
    pebble(emulator, "kill", timeout=60)
    time.sleep(2)
    install(emulator)
    time.sleep(6)
    subprocess.run(["pebble", "wipe"], capture_output=True, timeout=120)
    pebble(emulator, "kill", timeout=60)
    time.sleep(2)
    install(emulator)
    time.sleep(6)
    install(emulator)
    time.sleep(8)


def button(emulator, click):
    pebble(emulator, "emu-button", "click", click)
    time.sleep(1.5)


def hold(emulator, click, seconds=0.8):
    pebble(emulator, "emu-button", "push", click)
    time.sleep(seconds)
    pebble(emulator, "emu-button", "release", click)
    time.sleep(1.5)


def shot(emulator, path):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    result = pebble(emulator, "screenshot", "--no-open", path, timeout=90)
    if "Saved" not in result.stdout + result.stderr:
        raise SystemExit(f"FAIL: screenshot {path}: "
                         f"{result.stdout}{result.stderr}")
    return path


def read_png(path):
    """(width, height, pixels) - RGBA bytes, stdlib only."""
    data = open(path, "rb").read()
    pos, idat, width = 8, b"", None
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        if tag == b"IHDR":
            width, height, _, ctype = struct.unpack(">IIBB", payload[:10])
        elif tag == b"IDAT":
            idat += payload
        pos += 12 + length
    channels = {0: 1, 2: 3, 4: 2, 6: 4}[ctype]
    stride = width * channels
    raw, out, prev = zlib.decompress(idat), bytearray(), bytearray(stride)
    p = 0
    for _ in range(height):
        ftype, p = raw[p], p + 1
        line = bytearray(raw[p:p + stride])
        p += stride
        if ftype == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif ftype == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ftype == 3:
            for i in range(stride):
                a = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + (a + prev[i]) // 2) & 0xFF
        elif ftype == 4:
            for i in range(stride):
                a = line[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                pp = a + b - c
                pa, pb, pc = abs(pp - a), abs(pp - b), abs(pp - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF
        out += line
        prev = line
    return width, height, bytes(out)


def frame(path):
    return read_png(path)[2]


def dark_ratio(path):
    width, height, px = read_png(path)
    channels = len(px) // (width * height)
    dark = sum(1 for i in range(0, width * height * channels, channels)
               if px[i] < 40)
    return dark / (width * height)


def diff_ratio(path_a, path_b):
    """Fraction of pixels that differ between two captures of the same platform.

    For is_app= predicates on apps the darkness test cannot judge: capture a
    reference of where the emulator starts, then accept frames that differ from
    it. Too weak on its own for launch() - stepping through the launcher's own
    menus also changes the frame.
    """
    wa, ha, pa = read_png(path_a)
    wb, hb, pb = read_png(path_b)
    if (wa, ha) != (wb, hb):
        return 1.0
    channels = len(pa) // (wa * ha)
    step = channels
    total = wa * ha
    changed = sum(1 for i in range(0, len(pa), step) if abs(pa[i] - pb[i]) > 32)
    return changed / total


def launch(emulator, is_app=None, tries=5):
    """Bring the app to the front from wherever a fresh emulator starts.

    BACK twice clears whatever is up (including the app, which some platforms
    leave running after install), then SELECT starts it - straight away on
    platforms whose watchface launches the last app, through the launcher
    elsewhere. `is_app` gets the screenshot path and returns True when the app
    is on the glass; the default is "mostly dark", which the launcher (bright)
    and a light-on-dark app both satisfy - pass your own predicate for a
    predominantly light app.
    """
    is_app = is_app or (lambda path: dark_ratio(path) >= DARK)
    seen = []
    for _ in range(tries):
        button(emulator, "select")
        path = shot(emulator, os.path.join(PROBE_DIR, "_launch_probe.png"))
        if is_app(path):
            return path
        seen.append(f"{dark_ratio(path):.2f}")
    raise SystemExit(f"FAIL: never reached the app screen on {emulator} "
                     f"(darkness after each SELECT: {', '.join(seen)})")


def check_size(path, platform):
    """Raise unless the capture is the platform's own screen size."""
    want = PLATFORMS[platform]
    got = read_png(path)[:2]
    if got != want:
        raise SystemExit(f"FAIL: {path} is {got[0]}x{got[1]}, want "
                         f"{want[0]}x{want[1]}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} <platform> <out.png>\n"
                         f"platforms: {', '.join(PLATFORMS)}")
    platform, out = sys.argv[1], sys.argv[2]
    reset(platform)
    check_size(launch(platform), platform)
    check_size(shot(platform, out), platform)
    print(f"PASS - {out} ({PLATFORMS[platform][0]}x{PLATFORMS[platform][1]})")
