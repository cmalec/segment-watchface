# Part 5 — Timeline Quick View (unobstructed area)

**Goal:** when the system's Quick View overlay covers the bottom ~51 px, squeeze time/date/weather into the remaining space instead of hiding behind it.

## API

Poco exposes the overlay-aware dimensions directly:

| Property | Meaning |
|---|---|
| `render.width` / `render.height` | Full screen — use for **clearing the background** |
| `render.unobstructed.width` / `.height` | Area not covered by the overlay — use for **positioning content** |

No overlay → both are identical. One event instead of the C SDK's three callbacks:

```javascript
watch.addEventListener("resize", drawScreen);
```

## Move layout into the draw function

Constants computed at module scope (parts 2–4) are frozen at the startup screen size, so they never react to the overlay. Move them into `drawScreen()` and use the unobstructed height:

```javascript
function drawScreen(event) {
    const now = event?.date ?? lastDate;
    if (event?.date) lastDate = event.date;

    render.begin();
    render.fillRectangle(bgColor, 0, 0, render.width, render.height);   // full screen

    const blockHeight = timeFont.height + dateFont.height;
    const timeY = (render.unobstructed.height - blockHeight) / 2;      // visible area
    const dateY = timeY + timeFont.height;
    ...
}
```

Then swap every positioning use of `render.width`/`render.height` for `render.unobstructed.*`: battery bar width/centering, indicator Y, text X centering, weather Y (`render.unobstructed.height - smallFont.height - margin`).

## Alloy vs C

| | C | Alloy |
|---|---|---|
| Dimensions | `layer_get_unobstructed_bounds()` | `render.unobstructed.width/height` |
| Events | `.will_change`, `.change`, `.did_change` | single `resize` |
| Animation frames during the transition | yes (`.change`) | no — redraw once with the new dimensions |
| Startup state | manual initial call | always current |

## Verify

```bash
pebble build && pebble install --emulator emery
pebble emu-set-timeline-quick-view on --emulator emery
pebble screenshot --no-open --emulator emery shot-05-on.png
pebble emu-set-timeline-quick-view off --emulator emery
pebble screenshot --no-open --emulator emery shot-05-off.png
```

On: time/date/weather compressed into the visible band, battery bar inside the visible width. Off: original layout returns. Both screenshots read.

## Gotchas

- Quick View is not currently supported on the round platform (gabbro) — still write compliant code (`unobstructed` equals full size there).
- Redrawing in `resize` while a heavy fetch is in flight is fine, but don't start network work from `resize`.
- Background must stay `render.width`/`render.height`; using the unobstructed size leaves an unpainted strip and can show a stale frame.

Source: <https://developer.repebble.com/tutorials/alloy-watchface-tutorial/part5/>
