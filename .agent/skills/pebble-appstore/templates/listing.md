# <app> appstore submission

Everything the [Rebble developer portal](https://dev-portal.rebble.io/) asks for, in the order it asks for it. Fill this in, keep it next to the assets, and work down it when submitting. **Nothing is submitted until the portal steps at the bottom are done.**

The portal wants one *asset collection* per supported platform, so the description and the assets below are given per platform.

## Basic info

| Field                | Value                                                                                    |
|----------------------|------------------------------------------------------------------------------------------|
| Type                 | Watchapp / Watchface                                                                     |
| Title                | `<app>`                                                                                  |
| Category             | watchapp only: Daily, Games, Health & Fitness, Notifications, Remotes, Tools & Utilities |
| Source code URL      | `https://…`                                                                              |
| Website URL          | optional - leave blank if there is no project page                                       |
| Support email        | optional - leave blank to use the account email                                          |
| Release              | `releases/<app>-v<version>.pbw` (SDK `<version>`)                                        |
| UUID                 | `<uuid>`                                                                                 |
| Platforms            | aplite, basalt, chalk, diorite, emery, flint, gabbro                                     |
| Large icon (144x144) | `appstore/icon-144.png`                                                                  |
| Small icon (48x48)   | `appstore/icon-48.png`                                                                   |

Assets are generated, not hand-made, so the next release just reruns them (both need `pebble build` first):

```sh
python3 scripts/emulator.py <platform> appstore/screenshots/<platform>-results.png
python3 scripts/make_assets.py --screens appstore/screenshots --out appstore \
    --title <app> --tagline "<one line>" --cue "<chip>" --cue "<chip>"
python3 scripts/audit_assets.py appstore
```

## Description

Paste this as the description of every asset collection (it is under the 1600 character limit), then add the platform line for that collection.

```text
<description: what it does, the three or four things worth knowing, and
anything a buyer would otherwise have to ask>
```

| Platform | Line                                          |
|----------|-----------------------------------------------|
| aplite   | `<how the app looks/behaves on this display>` |
| basalt   | …                                             |
| chalk    | …                                             |
| diorite  | …                                             |
| emery    | …                                             |
| flint    | …                                             |
| gabbro   | …                                             |

## Asset collections

Screenshots are raw emulator frames at the platform's own resolution, the first one being the screen the app should be judged by. Banners are 720x320 and frame that platform's own screenshot. A watchapp collection also takes up to three header images (720x320) for the carousel; the banner works as the first one.

| Platform | Screenshot size | Screenshots                 | Banner               |
|----------|-----------------|-----------------------------|----------------------|
| aplite   | 144x168         | `screenshots/aplite-*.png`  | `banner/aplite.png`  |
| basalt   | 144x168         | `screenshots/basalt-*.png`  | `banner/basalt.png`  |
| chalk    | 180x180         | `screenshots/chalk-*.png`   | `banner/chalk.png`   |
| diorite  | 144x168         | `screenshots/diorite-*.png` | `banner/diorite.png` |
| emery    | 200x228         | `screenshots/emery-*.png`   | `banner/emery.png`   |
| flint    | 144x168         | `screenshots/flint-*.png`   | `banner/flint.png`   |
| gabbro   | 260x260         | `screenshots/gabbro-*.png`  | `banner/gabbro.png`  |

## Publishing, in portal order

1. Log in at <https://dev-portal.rebble.io/> and pick **Add a Watchapp** (or **Add a Watchface**).
2. Basic info from the table above: title, source code URL, category, icons.
3. **Add a release** and upload `releases/<app>-v<version>.pbw`, then publish the release (a listing is not complete until a release is published).
4. **Manage asset collections**: create one per platform listed above - the description with its platform line, the screenshots, the banner.
5. Publish the listing. Review the preview page before publishing.
6. Copy the public appstore link and the deep link once it is live.

## Before publishing

- [ ] Unique, valid UUID - never used by another app
- [ ] Built with a non-beta SDK, for every platform in `targetPlatforms`
- [ ] Release version higher than every published release
- [ ] `python3 scripts/audit_assets.py appstore` passes
- [ ] Screenshots reviewed by eye: right screen, nothing cropped by a round mask, no blank title on 1-bit platforms
- [ ] If a settings page is hosted (GitHub Pages or similar), it is pushed and the live URL matches the repo - the watch and the page must agree on what a setting means
- [ ] Read the portal's legal agreements while logging in
- [ ] After the release is public: install from the appstore on a real watch once, including any phone-app path (settings gear, companion app), which emulators cannot cover
