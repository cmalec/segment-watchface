---
name: pebble-appstore
description: |
  Get a Pebble/Rebble watchapp or watchface ready for the appstore: release build, the 144x144 and 48x48 icons, per-platform screenshots at the appstore's own sizes, a 720x320 banner per asset collection, the listing copy, and the pre-flight checklist for dev-portal.rebble.io. Use when the user asks to publish or submit an app to the Pebble or Rebble appstore, to prepare appstore assets, screenshots, icons or banners for a Pebble project, to check what a submission is still missing, or to fix rendering that only shows up in appstore screenshots.
---

# Pebble / Rebble appstore release

Everything between "the app works" and "the listing is live". The portal is at
<https://dev-portal.rebble.io/>; the guides behind this skill are
<https://developer.rebble.io/guides/appstore-publishing/> (overview, preparing a submission, publishing an app, appstore assets).

The portal wants a *release build*, two icons, and one **asset collection per supported platform** (description + up to 5 screenshots + a banner). Details, sizes and how they were established: [`reference.md`](reference.md).

## What a submission needs

| Item             | Value                                                                               |
|------------------|-------------------------------------------------------------------------------------|
| Release build    | a `.pbw` from a non-beta SDK, version above every published release                 |
| UUID             | unique, never reused from another app                                               |
| Large icon       | 144x144 PNG (the appstore's `list_image`)                                           |
| Small icon       | 48x48 PNG (the appstore's `icon_image`)                                             |
| Asset collection | per platform: description ≤1600 chars, ≤5 screenshots, banner                       |
| Screenshots      | PNG/GIF, device resolution for that platform, **unframed**                          |
| Banner           | 720x320 (watchapps may also add up to 3 header images, same size)                   |
| Category         | watchapp: Daily, Games, Health & Fitness, Notifications, Remotes, Tools & Utilities |

## Workflow

1. **Release build.** Bump `version` in `package.json`, `pebble build`, then check what the portal will read:
   `unzip -p build/<app>.pbw appinfo.json | jq '{name, versionLabel, uuid, targetPlatforms, capabilities}'`.
   `versionLabel` comes from `package.json`; `capabilities: ["configurable"]`
   is what makes the phone app show a settings gear. Copy the build to
   `releases/<app>-v<version>.pbw` and keep it in git.
2. **Assets, generated not hand-made.** Icons and banners are vector art rendered at exact sizes; screenshots are emulator captures. See
   `scripts/make_assets.py` and `scripts/emulator.py`.
3. **Screenshots per platform.** Capture the screens the listing should show, on every platform in `targetPlatforms`, from a factory-fresh emulator so the app is in its default state. `scripts/emulator.py` does the reset, launch and capture; drive the app-specific steps (open a menu, change a setting) with button presses.
4. **Listing copy.** One description (≤1600 chars) reused for every collection, plus a one-line platform note per collection, because the collections exist to tailor the listing per platform. `templates/listing.md`
   is the sheet to fill in and keep next to the assets.
5. **Pre-flight, then submit.** Run `scripts/audit_assets.py`, tick the checklist, then in the portal: Add a Watchapp → basic info + icons → upload and **publish** the release → create the asset collections → publish the listing.
6. **After publishing.** Copy the appstore and deep links, and install from the appstore on a real watch once: the phone-app paths (settings gear, companion app) are the ones emulators cannot cover.

## Emulator gotchas that cost real time

- **App data survives `pebble install`.** A settings change from an earlier session leaks into the screenshots. `pebble wipe` only clears a *running*
  emulator, so install first (which starts it), wipe, then install again.
- **After a wipe the app is missing from the launcher** until a second install; the launch flow below relies on that.
- **AppMessage can be dead on some emulators** (aplite). Never use messages to prove the app is in front or to set state; press buttons, and detect the app by the frame changing when you navigate.
- **Emulators drop button presses under load.** Never trust a fixed click sequence: press, capture, compare, and retry until the frame changes (or give the launch loop spare attempts).
- **Watchfaces vs watchapps differ in how they are fronted.** From a fresh emulator, either the installed app runs immediately or the watchface sits there; BACK twice then SELECT once or twice covers both.
- **1-bit platforms map colors by luminance, not intent.** `GColorOrange`
  renders black on aplite/diorite/flint — an orange title is invisible there. Check every platform's screenshots, not just the color ones.
- **Round displays crop with a circular mask.** Rows near the top and bottom of the glass get cut unless each row insets to the mask's chord; a listing screenshot with a sliced bottom row looks like a broken app. `reference.md`
  has the geometry.
- **A settings page hosted on GitHub Pages is part of the release.** If the app pushes settings to a web page, push that page *before* submitting, and verify the live URL matches the repo (`curl -sS <url> | cmp - config.html`), or the watch and the page will disagree about the settings it sends.
- **GitHub rejects pushes that expose a private email** (GH007). Use the account's noreply address for author and committer.

## Verifying

```sh
npm test && npm run test:emulator        # app still behaves (project-specific)
python3 scripts/audit_assets.py appstore # sizes, counts, description length
pebble build && cmp build/<app>.pbw releases/<app>-v<version>.pbw
```

The screenshots are the listing: open them. A capture tool that only checks
"the frame changed" will happily ship a cropped row, a blank title or the launcher instead of the app.

## Bundled files

| File                      | Use                                                            |
|---------------------------|----------------------------------------------------------------|
| `reference.md`            | portal fields, sizes per platform, pbw anatomy, CLI recipes    |
| `scripts/emulator.py`     | reset/launch/press/capture on a Pebble emulator (stdlib only)  |
| `scripts/make_assets.py`  | render icons and 720x320 banners from SVG (needs rsvg-convert) |
| `scripts/audit_assets.py` | check an `appstore/` tree against the sizes above              |
| `templates/listing.md`    | submission sheet to fill in: fields, copy, checklist           |

Worked example: the pl8s repo (`appstore/`, `tools/appstore_shots.py`,
`tools/make_appstore_assets.py`) is a complete submission built this way.
