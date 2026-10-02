# Segment — appstore submission

Everything the [Rebble developer portal](https://dev-portal.rebble.io/) asks for, in the order it asks for it. **Nothing is submitted until the portal steps at the bottom are done.**

Filled in from the skill's template (`.agent/skills/pebble-appstore/templates/listing.md`); assets are generated, not hand-made.

## Basic info

| Field                | Value                                                                 |
|----------------------|-----------------------------------------------------------------------|
| Type                 | Watchface                                                             |
| Title                | `Segment`                                                             |
| Category             | watchfaces are not categorised                                        |
| Source code URL      | `https://github.com/cmalec/segment-watchface`                         |
| Website URL          | blank — the settings page URL is in the description                   |
| Support email        | blank — the account email is used                                     |
| Release              | `releases/segment-v1.0.0.pbw` (SDK 4.33.1, non-beta; manifest `sdk_version` 5.106) |
| UUID                 | `c3c75c5a-f27b-4686-920f-0bd79ef71277`                                |
| Platforms            | emery (Pebble Time 2) — `targetPlatforms`                            |
| Large icon (144x144) | `appstore/icon-144.png`                                               |
| Small icon (48x48)   | `appstore/icon-48.png`                                                |

Assets regenerate from this repo (screenshots first — the banner embeds one):

```sh
pebble build
.agent/skills/pebble-appstore/scripts/emulator.py emery appstore/screenshots/emery-1.png
pebble emu-steps 4077 && pebble emu-heart-rate 72        # a plausible face, not a 0-step one
pebble send-app-message --uint 10009=0                   # default: seconds off
pebble screenshot --no-open appstore/screenshots/emery-1.png
pebble send-app-message --uint 10009=1
pebble screenshot --no-open appstore/screenshots/emery-2.png
python3 tools/make_appstore_assets.py
.agent/skills/pebble-appstore/scripts/audit_assets.py appstore --platforms emery
```

## Description

Paste this as the description of the asset collection, then add the platform line.

```text
Segment is a seven-segment digital watchface for the Pebble Time 2, in the spirit of the classic 91 Dub.

The clock is a large seven-segment readout with an optional seconds row. The row above it holds the date in five formats (DD/MM/YY, MM/DD/YY, YY-MM-DD, WED-25, SEP-WED-25) and follows the watch's own 12/24-hour setting.

Around it:
- Weather row: current temperature with the day's high and low, and an icon for the WMO condition from Open-Meteo — sun and moon at night, cloud, fog, drizzle, rain, showers, sleet, snow, thunderstorm. Fetched by the phone using its location; °C/°F is a setting.
- Health rows: steps (or last night's sleep until the day reaches 400 steps — a setting makes it always show steps), heart rate, and a thin phone-battery bar under the watch's own battery readout.
- Bottom strip: Chrono / V1 / Graph section labels, in the bundled face or Gothic, Bitham, Roboto Condensed or Leco.
- Chrome: the "pebble" branding toggle, custom button labels (LIGHT / PREV / NEXT by default), a blinking colon, hourly vibe, a buzz when the phone disconnects, a power-save window, and invert.

Colours: 205 presets carried over from 91 Dub, picked from the phone's settings page (the gear in the Pebble app), applied as you change them.

Everything running is on the watch: no companion app. The watchface uses the health permission for the rows above and the location permission to look up the weather; nothing else leaves the phone.
```

| Platform | Line |
|----------|------|
| emery    | Built for the Pebble Time 2's 200x228 display — the layout is native, not scaled from a smaller one. |

## Asset collections

| Platform | Screenshot size | Screenshots | Banner              |
|----------|-----------------|-------------|---------------------|
| emery    | 200x228         | `appstore/screenshots/emery-1.png` (default face), `emery-2.png` (seconds row) | `appstore/banner/emery.png` |

`emery-1.png` is the fresh-install state (steps set to 4,077 so the health row shows a real reading rather than the first boot's zero); `emery-2.png` adds the optional seconds row.

## Publishing, in portal order

1. Log in at <https://dev-portal.rebble.io/> and pick **Add a Watchface**.
2. Basic info from the table above: title, source code URL, icons.
3. **Add a release** and upload `releases/segment-v1.0.0.pbw`, then publish the release.
4. **Manage asset collections**: create the emery collection — description with its platform line, the two screenshots, the banner.
5. Publish the listing, after reading the preview page.
6. Copy the public appstore link and the deep link once it is live.

## Before publishing

- [x] Unique, valid UUID — `c3c75c5a-…`, this project's own
- [x] Built with a non-beta SDK (4.33.1) for every platform in `targetPlatforms` (emery only)
- [x] Release version (1.0.0) — nothing published before it
- [x] `audit_assets.py appstore --platforms emery` passes
- [x] Screenshots reviewed by eye — right screen, nothing cropped (rectangular display, no round mask to clear), no blank title
- [x] Settings page pushed and live at the URL the watch builds (`server/index.16.html` on GitHub Pages — the pkjs points at the same file)
- [ ] Portal legal agreements read while logging in
- [ ] Install from the appstore on a real watch once it is public, including the settings gear path (emulators cannot cover the phone-app side)
