# Part 2 — Custom Fonts and Centered Layout

**Goal:** replace system fonts with Jersey 10 (a TTF) and center the time+date block using real font metrics.

## 1. Get the font

Jersey 10 is a free Google Font. Place the TTF in the project:

```
src/embeddedjs/assets/Jersey10-Regular.ttf
```

`scripts/fetch_font.py` in this skill downloads it (sha256-pinned) into the right place:

```bash
python3 scripts/fetch_font.py /path/to/project
```

## 2. Declare the font resources (build time)

`src/embeddedjs/manifest.json` — the Moddable build converts the TTF into optimized bitmap resources, **once per size**:

```json
{
    "include": ["$(MODDABLE)/examples/manifest_mod.json"],
    "modules": { "*": "./main.js" },
    "resources": {
        "*-alpha": [
            { "source": "./assets/Jersey10-Regular", "size": 56, "monochrome": true, "blocks": ["Basic Latin"] },
            { "source": "./assets/Jersey10-Regular", "size": 24, "monochrome": true, "blocks": ["Basic Latin"] }
        ]
    }
}
```

| Property | Meaning |
|---|---|
| `source` | Path to the `.ttf` **without** extension, relative to the manifest |
| `size` | Pixel size to rasterize at (must match the size requested in JS) |
| `monochrome` | `true` → crisp 1-bit glyphs (ideal for Pebble); omit for anti-aliasing |
| `blocks` / `characters` | Character subset — include only what you draw; saves flash |

## 3. Load the font at runtime

```javascript
import parseBMF from "commodetto/parseBMF";
import parseRLE from "commodetto/parseRLE";

function getFont(name, size) {
    const font = parseBMF(new Resource(`${name}-${size}.fnt`));       // metrics
    font.bitmap = parseRLE(new Resource(`${name}-${size}-alpha.bm4`)); // pixel data
    return font;
}

const timeFont = getFont("Jersey10-Regular", 56);
const dateFont = getFont("Jersey10-Regular", 24);
```

The returned object behaves exactly like a system font: `drawText`, `getTextWidth`, `.height`.

## 4. Center the block with font metrics

```javascript
const blockHeight = timeFont.height + dateFont.height;
const timeY = (render.height - blockHeight) / 2;
const dateY = timeY + timeFont.height;
```

(Part 5 moves these into the draw function so they follow the unobstructed height.)

Draw both strings centered horizontally with `(render.width - render.getTextWidth(str, font)) / 2`.

## C vs Alloy custom fonts

| | C SDK | Alloy |
|---|---|---|
| Font file | `resources/fonts/` | `src/embeddedjs/assets/` |
| Declaration | `package.json` resources array | `manifest.json` `*-alpha` |
| Loading | `fonts_load_custom_font()` | `parseBMF()` + `parseRLE()` |
| Subsetting | all glyphs | `blocks` / `characters` |
| Cleanup | `fonts_unload_custom_font()` | GC |

## Verify

```bash
pebble build && pebble install --emulator emery
pebble screenshot --no-open --emulator emery shot-02.png
```

Expect the distinctive Jersey digits and a vertically centered time+date block.

## Gotchas

- **Size mismatch is fatal at runtime**, not at build time: requesting `getFont("Jersey10-Regular", 40)` when the manifest only declares 56/24 fails to find the resource. Declare every size you use.
- The `assets/` path is relative to the manifest, and the extension is dropped from `source`.
- Precompute what you can at startup — but anything derived from screen/unobstructed size belongs in the draw function (part 5).

Source: <https://developer.repebble.com/tutorials/alloy-watchface-tutorial/part2/>
