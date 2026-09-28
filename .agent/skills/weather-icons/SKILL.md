---
name: weather-icons
description: Use when working on the Segment watchface's weather row — the erikflowers weather-icons glyph font it draws conditions from. Covers the wiring (package.json font resource, WMO mapping, glyph table, day/night forms), how to add or change a condition, the silent failure modes (ellipsis-truncated layers, glyphs missing from the baked subset, stale font resource cache), and how to verify glyphs on the emulator. Repo-specific.
---

# Weather icons (Segment's weather row)

The row at the bottom of the face is `[icon] [19°(28°/12°)] … [seconds]`. The icon is **a glyph of the [Weather Icons](https://erikflowers.github.io/weather-icons/) font**
(`resources/fonts/weather-icons.ttf`, SIL OFL 1.1 — licence text next to it), not bitmap art and not PDC: one `TextLayer` whose text is a private-use-area codepoint.

Everything below was verified on the emery emulator (SDK 4.33.1, pebble tool v5.0.40) while this wiring was built — the failure modes in particular are the ones that actually bit.

## Where the pieces are

| Piece           | File                                                 | Note                                                   |
|-----------------|------------------------------------------------------|--------------------------------------------------------|
| Font            | `resources/fonts/weather-icons.ttf`                  | 99 KB in the repo; only the subset below ships         |
| Resource        | `package.json` → `WX_ICON_16`                        | `type: font`, `characterRegex` lists the 12 codepoints |
| Glyph table     | `src/c/decorations.c` → `weather_glyphs[][]`         | condition → {day, night} UTF-8 bytes                   |
| WMO mapping     | `src/c/helpers.c` → `weather_cond_from_wmo()`        | host-tested in `tests/host/test_helpers.c`             |
| Day/night split | `src/c/decorations.c` → `WEATHER_NIGHT_START`/`_END` | 19:00 → 07:00; no sunrise data on the watch            |
| Frame           | `src/c/_globals.h` → `DECORATIONS_WEATHER_ICON`      | 24×22: must fit the widest advance *and* the ink band  |
| Font load       | `src/c/fonts.c` → `font_weather`                     | loaded in `fonts_init()` before `decorations_init()`   |
| Verification    | `tools/verify_weather_glyphs.py`                     | see below                                              |

The face draws ten conditions: clear, partly cloudy, cloudy, fog, drizzle, rain, showers, sleet, snow, thunderstorm. Clear and partly cloudy have sun and moon forms; the rest use the font's neutral cloud-with-precipitation glyphs, which read the same day and night. Anything outside the WMO table draws nothing rather than a wrong icon.

## How the SDK turns the .ttf into the shipped font

- The **pixel height comes from a number in the resource name**: `WX_ICON_16` → 16 px. (`resource_generator_font.py::_get_font_height_from_name`; a `pixelHeight`
  field can override it.)
- `characterRegex` is a Python regex matched against `chr(codepoint)`, so a character class of literal PUA characters selects exactly the glyphs to bake. Twelve glyphs cost **~1.6 KB** of resource; the webfont's other 210 glyphs do not ship.
- Glyph codepoints are the ones the vendor's CSS declares: `.wi-day-sunny:before
  { content: "\f00d"; }` etc. Look them up in the upstream `css/weather-icons.css`
  (class → codepoint), not from memory.

## Adding or changing a condition

1. Pick the class in the vendor CSS, take its codepoint (e.g. `wi-hail` → `\uf015`).
2. Add the UTF-8 bytes to `weather_glyphs[][]` in `decorations.c`:
   `U+F015` → `"\xEF\x80\x95"` (the three bytes are `0xE0|hi`, `0x80|…`, `0x80|lo`
   of the codepoint — the table's existing rows show the pattern).
3. Add the codepoint to `characterRegex` in `package.json` as `\uf015`.
4. Map the WMO code (s) to the new condition in `weather_cond_from_wmo()` and cover them in `tests/host/test_helpers.c`.
5. **`pebble build distclean && pebble build configure && pebble build`** — a
   `characterRegex` change alone does not rebuild the font resource (the cache is keyed on the file, not the definition), so the new glyph would silently not exist and the watch would draw the wildcard glyph instead.
6. `tests/run.sh`, then `tools/verify_weather_glyphs.py`.

Changing the *size* means renaming the resource (`WX_ICON_15`, …) — and then the frame in `_globals.h` and the two layout invariants in `tests/host/test_layout.c`
need a look, because the ink band scales with the font size.

## Failure modes (all silent)

| Symptom                                                                                          | Cause                                                                                                                                                                                                                                                                                          |
|--------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| The icon is the font's **wildcard glyph** (a filled vertical rectangle) or the system font's `…` | Codepoint is not in the baked font: missing from `characterRegex`, a glyph the `.ttf` doesn't carry, or a stale font resource after a regex edit (see step 5).                                                                                                                                 |
| The icon is **three dots** ("...") in the right place                                            | The `TextLayer` frame is narrower than the glyph's advance: Pebble's text layout draws a truncated layer as an ellipsis. This is what 21–22 px glyphs did in a 20 px box. Widen `DECORATIONS_WEATHER_ICON` instead of shrinking the font — the widest glyph in the shipped set advances 24 px. |
| The icon's top or bottom rows are **cut**                                                        | The frame is shorter than the glyph ink. The font's line box is `pixel_height`, but glyph ink runs to its 21st row (and `day-cloudy` 1 px above it). The 22 px band in `_globals.h` covers all of it except that one documented pixel.                                                         |
| Nothing redraws at dusk                                                                          | `decorations_weather_icon_tick()` is what swaps sun ↔ moon; it is called from the clock's `MINUTE_UNIT` branch in `src/c/timedigits.c`.                                                                                                                                                        |
| Icon ignores a colour change                                                                     | `weather_icon_apply()` sets the ink colour (`colors[c_t2]` through `color_helper(..., Invert)`); it is called from `decorations_settings_callback()`.                                                                                                                                          |

## Verifying on the emulator

`tools/verify_weather_glyphs.py` is the whole recipe: it parses the glyph table and the icon rect out of the C sources, decodes the glyph bitmaps from the *built* font resource, drives one AppMessage per condition (`pebble send-app-message` with the numeric key ids read from `build/src/message_keys.auto.c`), screenshots each, and compares the ink pixel-for-pixel.

```sh
pebble build && pebble install --emulator emery
pebble emu-bt-connection --emulator emery --connected no   # keep the phone's real
                                                           # weather from over-
                                                           # writing the injection
tools/verify_weather_glyphs.py --phase day
tools/verify_weather_glyphs.py                             # night forms after dark
pebble emu-bt-connection --emulator emery --connected yes
```

Notes that save time:

- `--phase night` refuses to run during the day: the face derives the phase from the clock and there is no way to lie to it from outside except by changing the hours. To test the moon forms out of hours, temporarily force
  `weather_is_night()` to `return true`, verify, then **revert** (and re-verify the day forms) — don't leave it in.
- `pebble emu-set-time` does not stick: the emulator syncs the watch clock back from the host within seconds, so a night screenshot taken that way is a race.
- Injecting one condition and screenshotting immediately can catch the *previous*
  glyph (AppMessage latency). The verifier's 1.6 s pause is tuned for the emulator; if a row looks stale, raise it.
- Byte-level cross-check of what was baked: `build/emery/resources/fonts/*.reso` is a pickled `ResourceObject` (unpickle with a shim, then read its `data` as a v3 font bitstring: info, hash table, offset tables, glyph table — the SDK's
  `tools/font/fontgen.py` documents the layout).

## Licence

The font is SIL OFL 1.1: the licence text and the attribution (icon designs by Lukas Bischoff, art for v1.1+ by Erik Flowers) live in
`resources/fonts/weather-icons.LICENSE.txt` and are referenced from the README. Keep that file with the font — it is a redistribution requirement, not decoration.
