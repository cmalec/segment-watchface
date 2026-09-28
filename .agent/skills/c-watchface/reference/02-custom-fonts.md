# Part 2 — Custom Fonts and Centered Layout

**Goal:** replace system fonts with Jersey 10 (a TTF) and center the time+date block.

## 1. Font file

A custom font must be TrueType (`.ttf`). Place it under `resources/`:

```
resources/fonts/Jersey10-Regular.ttf
```

`scripts/fetch_font.py` in this skill downloads it (sha256-pinned) to that path:

```bash
python3 scripts/fetch_font.py /path/to/project
```

## 2. Register the resource (build time)

`package.json` — the same file registered twice, once per size you want (the number in the name is just a reminder):

```json
"resources": {
  "media": [
    { "type": "font", "name": "FONT_JERSEY_56", "file": "fonts/Jersey10-Regular.ttf", "compatibility": "2.7" },
    { "type": "font", "name": "FONT_JERSEY_24", "file": "fonts/Jersey10-Regular.ttf", "compatibility": "2.7" }
  ]
}
```

`name` becomes `RESOURCE_ID_FONT_JERSEY_56` in C. Fonts need `"compatibility": "2.7"` (the SDK version that introduced `fonts_load_custom_font`), otherwise the build refuses the resource.

## 3. Load in C

```c
static GFont s_time_font;
static GFont s_date_font;

// in main_window_load():
s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_56));
s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_24));

text_layer_set_font(s_time_layer, s_time_font);
text_layer_set_font(s_date_layer, s_date_font);
```

## 4. Center the block

```c
int date_height = 30;
int block_height = 56 + date_height;                       // 56 ≈ the tall font's line height
int time_y = (bounds.size.h / 2) - (block_height / 2) - 10; // -10 compensates font ascent padding
int date_y = time_y + 56;

s_time_layer = text_layer_create(GRect(0, time_y, bounds.size.w, 60));
s_date_layer = text_layer_create(GRect(0, date_y, bounds.size.w, 30));
```

The `- 10` offset exists because custom fonts carry internal ascent padding that pushes glyphs down; tune it per font.

## 5. Clean up

```c
static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);

  // Always destroy layers BEFORE unloading the fonts they use
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
}
```

## C vs Alloy custom fonts

| | C | Alloy |
|---|---|---|
| Font file | `resources/fonts/` | `src/embeddedjs/assets/` |
| Declaration | `package.json` `resources.media` | `manifest.json` `*-alpha` |
| Loading | `fonts_load_custom_font()` + `resource_get_handle()` | `parseBMF()` + `parseRLE()` |
| Size handling | one resource per rendered size (the `.ttf` is rasterized at build) | same (one resource per `size`) |
| Cleanup | `fonts_unload_custom_font()` | GC |

## Verify

```bash
pebble build && pebble install --emulator emery
pebble screenshot --no-open --emulator emery shot-02.png
```

Expect the distinctive Jersey digits and a vertically centered time+date block.

## Gotchas

- Registering the same TTF twice is intentional: each `media` entry rasterizes the font at one size.
- Unloading a font while a layer still references it crashes on the next draw — destroy layers first.
- Pixel/bitmap-style fonts stay legible at small sizes; text fonts get muddy on Pebble displays.

Source: <https://developer.repebble.com/tutorials/watchface-tutorial/part2/>
