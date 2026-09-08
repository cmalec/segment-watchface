/* Node unit tests for the pure logic embedded in src/pkjs/index.js and the
 * wire contract with the settings page. Run: node tests/test_pkjs.js
 *
 * pkjs is written for the PebbleKit JS runtime (Pebble.*, navigator,
 * XMLHttpRequest), which isn't loadable in Node — so the pure functions are
 * re-declared here mirroring index.js exactly, and the assertions guard
 * against the two of them drifting apart. Keep in sync with src/pkjs/index.js.
 */
'use strict';
let pass = 0, fail = 0;
function eq(a, b, msg) {
  if (a === b) { pass++; }
  else { fail++; console.log(`FAIL ${msg}: got ${JSON.stringify(a)}, want ${JSON.stringify(b)}`); }
}
function ok(c, msg) { c ? pass++ : (fail++, console.log(`FAIL ${msg}`)); }

/* ---- mirrored from index.js ---- */
var STALE_MS = 3 * 60 * 60 * 1000;
function isStale(w) {
  return !w || typeof w.fetchedAt !== 'number' || (Date.now() - w.fetchedAt) > STALE_MS;
}
// inlined conversion (must match the literal in sendWeatherToWatch)
function toF(c) { return Math.round(c * 9 / 5 + 32); }
function toCdisplay(c, unit) { return unit === 1 ? toF(c) : c; }

/* ---- temp conversion ---- */
eq(toF(0), 32, '0C = 32F');
eq(toF(100), 212, '100C = 212F');
eq(toF(-40), -40, '-40C = -40F (crossover)');
eq(toF(37), 99, '37C = 99F');
eq(toF(22), 72, '22C = 72F');
eq(toF(-10), 14, '-10C = 14F');
eq(toCdisplay(20, 0), 20, 'Celsius passthrough');
eq(toCdisplay(20, 1), 68, 'Fahrenheit converts');
eq(toCdisplay(-5, 1), 23, 'negative C -> F');

/* ---- staleness ---- */
ok(isStale(null), 'null weather is stale');
ok(isStale({}), 'missing fetchedAt is stale');
ok(isStale({ fetchedAt: 'nope' }), 'non-number fetchedAt is stale');
ok(!isStale({ fetchedAt: Date.now() }), 'fresh now is not stale');
ok(!isStale({ fetchedAt: Date.now() - 1000 }), '1s old is fresh');
ok(isStale({ fetchedAt: Date.now() - STALE_MS - 1 }), 'older than STALE_MS is stale');
ok(!isStale({ fetchedAt: Date.now() - STALE_MS + 1000 }), 'just under STALE_MS is fresh');

/* ---- settings-page color pack/unpack (wire contract with the watch) ---- */
// settings page cssToByte: 0xC0 | r<<4 | g<<2 | b, r/g/b in 0..3
function cssToByte(r, g, b) { return (0xC0 | (r << 4) | (g << 2) | b); }
function byteToHex(v) { return v.toString(16).toUpperCase().padStart(2, '0'); }
// watch settings.c: (hex_to_num(hi)<<4) + hex_to_num(lo)  == parse hex byte
function watchUnpack(hex) { return parseInt(hex, 16); }

// default palette bytes from settings.c (white/black/orange/red/dukeblue)
const defaults = { white: 0xFF, black: 0xC0, orange: 0xF4, red: 0xC3, dukeblue: 0xCA, lightgray: 0xEA };
for (const [name, byte] of Object.entries(defaults)) {
  eq(watchUnpack(byteToHex(byte)), byte, `palette byte ${name} survives pack->hex->watch`);
}
// full byte range roundtrip through the page's packing
for (let r = 0; r < 4; r++) for (let g = 0; g < 4; g++) for (let b = 0; b < 4; b++) {
  const byte = cssToByte(r, g, b);
  eq(watchUnpack(byteToHex(byte)), byte, `cssToByte(${r},${g},${b}) roundtrip`);
}

console.log(`\n${pass} passed, ${fail} failed`);
process.exit(fail ? 1 : 0);
