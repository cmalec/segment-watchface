# Part 3 — Battery Meter and Bluetooth Alert

**Goal:** color-coded battery bar at the top, red "X" when the phone link drops.

## Battery

```javascript
import Battery from "embedded:sensor/Battery";

let batteryPercent = 100;

const battery = new Battery({
    onSample() {
        batteryPercent = this.sample().percent;   // 0–100
        drawScreen();                             // redraw on change
    }
});
batteryPercent = battery.sample().percent;        // initial value
```

`sample()` returns `{ percent, charging, plugged }`. Keep the instance open for the lifetime of the app (unlike Location, it is a continuous sensor — never `close()` it).

## Battery bar

```javascript
const green  = render.makeColor(0, 170, 0);
const yellow = render.makeColor(255, 170, 0);
const red    = render.makeColor(255, 0, 0);

function drawBatteryBar() {
    const barWidth  = (render.width / 2) | 0;
    const barX      = ((render.width - barWidth) / 2) | 0;
    const barY      = render.height < 180 ? 6 : 20;
    const barHeight = 8;

    // Border (white) + inner (background) = hollow bar
    render.fillRectangle(white, barX, barY, barWidth, barHeight);
    render.fillRectangle(black, barX + 1, barY + 1, barWidth - 2, barHeight - 2);

    const barColor = batteryPercent <= 20 ? red : batteryPercent <= 40 ? yellow : green;
    const fillWidth = ((batteryPercent * (barWidth - 4)) / 100) | 0;
    render.fillRectangle(barColor, barX + 2, barY + 2, fillWidth, barHeight - 4);
}
```

`| 0` truncates to int (cheaper than `Math.floor` on device). Both supported platforms are ≥180 px tall, so they take the `20` offset — keep the branch if you add a small platform later.

## Connection state

```javascript
let isConnected = true;

function checkConnection() {
    isConnected = watch.connected.app;   // true = phone app reachable
    drawScreen();
}
watch.addEventListener("connected", checkConnection);
checkConnection();                       // `connected` does NOT fire on registration — check once at startup
```

Indicator, drawn under the battery bar:

```javascript
if (!isConnected) {
    const btWidth = render.getTextWidth("X", smallFont);
    render.drawText("X", smallFont, red, (render.width - btWidth) / 2,
                    render.height < 180 ? 16 : 30);
}
```

## Redraws without a time event

Battery/BT callbacks call `drawScreen()` with no event, so keep the last known date:

```javascript
let lastDate = new Date();

function drawScreen(event) {
    const now = event?.date ?? lastDate;
    if (event?.date) lastDate = event.date;
    ...
}
watch.addEventListener("minutechange", drawScreen);
```

## Verify

```bash
pebble build && pebble install --emulator emery
pebble emu-battery --percent 80 --emulator emery   # bar green
pebble emu-battery --percent 30 --emulator emery   # bar yellow
pebble emu-battery --percent 10 --emulator emery   # bar red
pebble emu-bt-connection --connected no --emulator emery    # red X appears
pebble emu-bt-connection --connected yes --emulator emery   # X disappears
```

Screenshot after each change and read the image.

## Gotchas

- Rendering the bar as border-then-inner-rect is the idiom for hollow shapes (Poco has no stroked rect with arbitrary inset).
- Since `connected`/battery callbacks redraw out of band, any state computed from `event` must have a fallback — that's what `lastDate` is for.
- `smallFont` used by the indicator: system font sizes only (`Gothic-Regular` 18 is valid).

Source: <https://developer.repebble.com/tutorials/alloy-watchface-tutorial/part3/>
