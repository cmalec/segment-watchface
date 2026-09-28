# Part 1 — Your First Watchface (Poco, time + date)

**Goal:** black-background watchface showing `HH:MM` and `Mon Jan 01`, redrawn once a minute.
**Produces:** `main.js` with a Poco renderer, two fonts, two colors, one draw function, one event listener.

## Steps

1. Create the project — `pebble new-project --alloy watchface` (or copy `templates/watchface` from this skill).
2. Make it a watchface in `package.json`:

```json
"watchapp": { "watchface": true }
```

Watchfaces are the default watch display: Up/Down are reserved for the timeline, Select belongs to the launcher, so **a watchface gets no button input** (accelerometer taps only). Interactive apps set `"watchface": false`.

3. `src/embeddedjs/main.js`:

```javascript
import Poco from "commodetto/Poco";

const render = new Poco(screen);            // `screen` global — the watch display

// Fonts and colors are set up ONCE at startup
const timeFont = new render.Font("Bitham-Bold", 42);
const dateFont = new render.Font("Gothic-Bold", 24);
const black = render.makeColor(0, 0, 0);
const white = render.makeColor(255, 255, 255);

const DAYS = ["Sun","Mon","Tue","Wed","Thu","Fri","Sat"];
const MONTHS = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"];

function draw(event) {
    const now = event.date;                 // provided by the event — no new Date()
    render.begin();
    render.fillRectangle(black, 0, 0, render.width, render.height);

    const hours = String(now.getHours()).padStart(2, "0");
    const minutes = String(now.getMinutes()).padStart(2, "0");
    const timeStr = `${hours}:${minutes}`;

    let width = render.getTextWidth(timeStr, timeFont);
    render.drawText(timeStr, timeFont, white, (render.width - width) / 2,
                    (render.height / 2) - timeFont.height + 5);

    const dateStr = `${DAYS[now.getDay()]} ${MONTHS[now.getMonth()]} ${String(now.getDate()).padStart(2, "0")}`;
    width = render.getTextWidth(dateStr, dateFont);
    render.drawText(dateStr, dateFont, white, (render.width - width) / 2,
                    (render.height / 2) + 10);

    render.end();
}

watch.addEventListener("minutechange", draw);   // fires immediately → initial draw
```

## Key APIs

| API | Notes |
|---|---|
| `new Poco(screen)` | Renderer; `render.width/height`, `render.begin()/end()` |
| `new render.Font(name, size)` | System font; **name+size must be a real combination** (see SKILL pitfalls) |
| `render.makeColor(r, g, b)` | 0–255 channels → display-optimized color value |
| `render.fillRectangle(color, x, y, w, h)` | Full-screen clear = `0, 0, render.width, render.height` |
| `render.getTextWidth(str, font)` | Pixels; use for horizontal centering |
| `font.height` | Line height; use for vertical layout |
| `watch.addEventListener("minutechange", cb)` | Fires on registration + every minute; the event carries `.date` |

## Verify

```bash
pebble build && pebble install --emulator emery
pebble screenshot --no-open --emulator emery shot-01.png   # read the image
```

Expect centered time and date on black. `pebble emu-set-time` shifts the clock to prove the redraw.

## Gotchas

- Don't call `new Date()` inside `draw` — the event's `date` is authoritative (and matches watch settings).
- `secondchange` also exists but redraws 60x more often; use it only for a requested seconds display.
- `padStart` matters: `9:5` vs `09:05`.

Source: <https://developer.repebble.com/tutorials/alloy-watchface-tutorial/part1/>
