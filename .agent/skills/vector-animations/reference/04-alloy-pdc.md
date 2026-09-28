# Alloy (Poco) and cross-platform use

Alloy can draw the same PDC files through Poco — the asset format is identical, only the loading and drawing calls differ.

**Verification status: the C route (this skill's template) was built and rendered on emery, gabbro and aplite. The Alloy route below comes from the SDK typings (`toolchain/moddable/typings/pebble/poco.d.ts`) and the Poco implementation (`build/devices/pebble/modules/poco/poco-pebble.js`), not from a run — treat it as a starting point and confirm with a screenshot.**

## API (from the SDK typings)

```javascript
import Poco from "commodetto/Poco";
const render = new Poco(screen);

const image = new Poco.PebbleDrawCommandImage(id);      // id is a NUMBER (see caveat)
const sequence = new Poco.PebbleDrawCommandSequence(id);

render.drawDCI(image, 10, 10);        // drawDCI(dci, x, y)
render.drawDCI(sequence, 10, 10);     // sequences draw through the same call
```

| Member | Meaning |
|---|---|
| `image.width` / `image.height` | bounds of the vector art |
| `image.scale(x, y)` / `.scale(f)` | scale the art (returns `this`) |
| `image.rotate(angle, cx, cy)` | rotate around a point |
| `image.process(cb)` | iterate the command list (`PebbleDrawCommand`: `type`, `strokeWidth`, `stroke`, `fill`, `hidden`) |
| `image.clone()` | editable copy |
| `sequence.width` / `.height` | bounds |
| `sequence.duration` / `.frameDuration` | total and per-frame duration in ms |
| `sequence.time` | current playback time (settable) |
| `sequence.clone()` | editable copy |

```javascript
// frame stepping: advance the sequence's own clock and redraw
sequence.time = (sequence.time + 50) % sequence.duration;
render.drawDCI(sequence, 10, 10);
```

## Caveat: the constructor wants a numeric resource id

```javascript
// poco-pebble.js
Poco.PebbleDrawCommandImage = class extends Native(...) {
    constructor(id) {
        ...
        id = Number(id);        // a filename string becomes NaN
```

Unlike fonts (`new Resource("Jersey10-Regular-56.fnt")`), PDC objects take what the implementation converts with `Number()` — so pass the resource's **integer id**, not a file name. No documented name→id helper was found in the SDK; if the numeric id is not reachable from JS in your project, use the C route for PDC assets in Alloy-based apps (a small C module behind FFI, or ship the icon as a bitmap via `new Poco.PebbleBitmap(...)`), or check the current Alloy documentation before designing around it.

## Differences to keep in mind

| | C | Alloy |
|---|---|---|
| Resource declaration | `package.json → resources.media` with `"type": "raw"` | `manifest.json` resources |
| Loading | `gdraw_command_image_create_with_resource(RESOURCE_ID_*)` | `new Poco.PebbleDrawCommandImage(<numeric id>)` |
| Drawing | `gdraw_command_image_draw(ctx, img, origin)` | `render.drawDCI(dci, x, y)` |
| Frame selection | `gdraw_command_sequence_get_frame_by_index()` / `_by_elapsed()` | `sequence.time = ms` then `drawDCI` |
| Runtime transform | `gdraw_command_set_point/…`, `set_bounds_size()` | `.scale()`, `.rotate()`, `.process()` on a clone |
| Cleanup | `gdraw_command_*_destroy()` | garbage collected |
| Platforms | all SDK targets (verified aplite, emery, gabbro) | emery and gabbro only |

Related skills: `alloy-watchface` (Poco rendering, watch-side JS), `c-watchface` (native watchface patterns), `pebble-watchface` (general API/asset reference).
