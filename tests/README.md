# Tests

Host-side unit tests for the watchface's pure logic. No Pebble SDK or ARM
toolchain needed — the tested code compiles against `tests/host/pebble.h`, a
minimal mock of the SDK (in-memory persist, stubbed AppMessage/timers).

## Run

```sh
tests/run.sh        # C logic (helpers, settings ranges, colors, layout)
node tests/test_pkjs.js   # phone-side pkjs logic (temp conversion, staleness, colors)
```

## Coverage

| Suite | Platform(s) | What it guards |
|-------|-------------|----------------|
| `test_helpers_emery` | emery | `duration_to_time`, `format_commas` (steps grouping), `hex_to_num` |
| `test_settings_emery` | emery | `setting_is_set2` / `setting_is_power_save` half-hour window math, incl. overnight wrap and end-exclusive boundary |
| `test_colors_emery` | emery | settings-page GColor8 hex ⇄ watch `hex_to_num` unpack round-trip (the B1 wire contract) |
| `test_layout_emery` | emery | native 200×228 bounds and seconds-mode frame geometry — full font line boxes, on-screen digits, separator ordering, and non-overlapping seconds |
| `test_pkjs.js` | node | `isStale` weather cache, `Math.round(c*9/5+32)` °F conversion, cssToByte/hex color round-trip |

## Adding a test

- Pure C function → declare it in a `test_*.c`, link the real `src/c/*.c`, stub
  any UI/weather symbols it references. Define the platform with
  `-DPBL_PLATFORM_<APLITE|EMERY|...>` and add a line to `tests/run.sh`.
- pkjs logic → add to `test_pkjs.js`. pkjs can't be `require()`d in Node (it
  uses the `Pebble` global), so mirror the function under test and keep it in
  sync with `src/pkjs/index.js`.

The on-watch rendering, animations, and hardware services (Health, BT,
battery) can't be covered by host tests — those are verified in the QEMU
emulator (`pebble install --emulator emery` + `pebble screenshot`).
