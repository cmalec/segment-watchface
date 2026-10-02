# Appstore reference

Facts behind the skill: what the portal asks for, the exact sizes, how to read a `.pbw`, and the emulator/CLI recipes. Sources are the Rebble guides and the appstore frontend itself (`https://apps.rebble.io/en_US/…`, whose bundle contains the per-platform mappings quoted below).

## Portal fields

**Basic info** (watchapp): title, website URL (optional), source code URL, support email (optional - blank means the account email is used), category, large and small icons.

**Asset collection**, one per supported platform:

| Field            | Limit                                                                 |
|------------------|-----------------------------------------------------------------------|
| Description      | 1600 characters                                                       |
| Screenshots      | 5 per platform, PNG/GIF/animated GIF, **device resolution, unframed** |
| Header images    | watchapps only, up to 3, 720x320                                      |
| Marketing banner | 720x320 (the docs' own banner template is 720x320)                    |

**Release**: at least one published `.pbw`; the version must be greater than every published release; UUID unique; SDK non-beta. Releases can be published privately while the app is not public yet - once public, an app cannot go back to private.

**Companion apps** and **timeline** are separate portal sections, needed only if the app has a phone companion app or pushes timeline pins. iOS app whitelisting (<https://developer.rebble.io/guides/appstore-publishing/whitelisting/>)
only matters for a companion app, not for the watchapp itself.

## Sizes per platform

The appstore serves screenshots and icons at fixed sizes per hardware; the mapping is in the frontend bundle as `SCREENSHOT_BY_HARDWARE` and the
`icon_image` / `list_image` / `header_images` sizes.

| Platform | Watch                    | Screenshot | Large icon (`list_image`) | Small icon (`icon_image`) |
|----------|--------------------------|------------|---------------------------|---------------------------|
| aplite   | Pebble / Pebble Steel    | 144x168    | 144x144                   | 48x48                     |
| basalt   | Pebble Time / Time Steel | 144x168    | 144x144                   | 48x48                     |
| chalk    | Pebble Time Round        | 180x180    | 144x144                   | 48x48                     |
| diorite  | Pebble 2                 | 144x168    | 144x144                   | 48x48                     |
| emery    | Pebble Time 2            | 200x228    | 144x144                   | 48x48                     |
| flint    | Pebble 2 Duo             | 144x168    | 144x144                   | 48x48                     |
| gabbro   | Pebble Time Round 2      | 260x260    | 144x144                   | 48x48                     |

A collection is required for **every** platform in `targetPlatforms`; the appstore picks the collection matching the user's hardware, so screenshots that show the app's real rendering per display class (color vs 1-bit, rect vs round, compact vs large) are the point of the per-platform split.

## Categories

Watchapp: `Daily`, `Games`, `Health & Fitness`, `Notifications`, `Remotes`,
`Tools & Utilities`. Watchfaces are not categorised (`Faces` is a browse section, not a category).

## The `.pbw`

A zip. Root holds `appinfo.json` and the phone-side JS (`pebble-js-app.js`
when `enableMultiJS`); one directory per platform holds `pebble-app.bin`,
`app_resources.pbpack` and `manifest.json`.

```sh
unzip -p build/myapp.pbw appinfo.json | jq '{
  name, displayName, versionLabel, uuid, sdkVersion, targetPlatforms,
  capabilities, watchapp, messageKeys, media: (.resources.media | length) }'
```

- `versionLabel` is the release version the portal reads - it comes from
  `package.json`'s `version`, not from `appinfo.json`'s other fields.
- `capabilities: ["configurable"]` is what shows the settings gear in the phone app.
- `resources.media` entries with `"menuIcon": true` are the launcher icon.
- `messageKeys` are generated from `package.json`; the runtime copy the JS side loads is `build/js/message_keys.json`.

Size check after a build: the build log's `memory_usage_report` per platform (RAM footprint vs the platform's limit) - a 1-bit platform has 24 KB of RAM and 128 KB of flash, so an app that installs on emery can still be too big for aplite.

## CLI recipes

```sh
pebble build                                   # all targetPlatforms
pebble install --emulator emery                # boots the emulator if needed
pebble emu-button click select                 # up | down | select | back
pebble emu-button push select && sleep 0.8 && pebble emu-button release select
pebble screenshot --no-open shot.png           # device-resolution PNG
pebble send-app-message --int 10000=5 10001=315
pebble wipe                                    # clears the RUNNING emulator only
pebble kill                                    # stops every emulator
pebble analyze-size                            # per-ELF sections (often all zero)
```

Message keys for `send-app-message --int` come from `build/js/message_keys.json`
after a build; one `--int` flag carries all `key=value` pairs.

## Round-mask geometry

On a round display of size `w` x `h` (square: `w == h`), the visible half width at height `y` is `sqrt(r^2 - (y - h/2)^2)` with `r = w/2`. Inset a row's content by `r - half_width` so its widest point stays on the glass; compute it per row from the edge of the row band furthest from the centre, and stop the row stack short of the bottom, where the chord is too narrow to hold anything. Integer square root by Newton's method keeps this SDK-free:

```c
static int isqrt(int n) {
  if (n <= 0) return 0;
  int x = n;
  for (int i = 0; i < 16; i++) {
    int next = (x + n / x) / 2;
    if (next >= x) break;
    x = next;
  }
  return x;
}
```

## Emulator behaviour worth knowing

- `pebble install` does not clear app storage; `pebble wipe` clears the *running* emulator's data. Order: kill → install (starts it) → wipe → kill → install → install, then the app is back in the launcher and running with defaults.
- The first install after a wipe leaves the app out of the launcher; a second install registers it.
- AppMessage delivery to the emulator is unreliable on some platforms (aplite never delivered); use button presses to drive and frame changes to verify.
- Screen detection without OCR: the app's own frame is the signal. `emulator.py`
  treats a frame as the app when it is at least 0.4 dark - the launcher is ~0.02,
  a watchface with a big light panel is still ~0.4-0.5, an all-black app screen is
  ~1.0. A predominantly *light* app needs its own `is_app=` predicate: capture a
  reference of where the emulator starts and use `diff_ratio()`; that alone is too
  weak for the default (stepping through the launcher's menus also changes the
  frame).
