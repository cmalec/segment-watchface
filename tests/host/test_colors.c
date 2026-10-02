/* Round-trip test: the settings page packs colors as GColor8 hex nibbles;
 * settings.c unpacks them via hex_to_num. Verify the wire contract holds. */
#include "pebble.h"
#include "test_util.h"

uint8_t hex_to_num(char h);

/* Mirror of the page's cssToByte: 0xC0 | r<<4 | g<<2 | b, r/g/b in 0..3 */
static uint8_t pack(int r, int g, int b) { return 0xC0 | (r << 4) | (g << 2) | b; }

/* Mirror of the page's serialization: two uppercase hex chars per byte */
static void to_hex(uint8_t v, char *out) { snprintf(out, 3, "%02X", v); }

/* settings.c's unpacking (identical expression) */
static uint8_t unpack(const char *pair) {
  return (hex_to_num(pair[0]) << 4) + hex_to_num(pair[1]);
}

static void test_color_roundtrip(void) {
  struct { int r, g, b; } cases[] = {
    {0,0,0}, {3,3,3}, {3,0,0}, {0,3,0}, {0,0,3}, {2,1,3}, {1,2,0}
  };
  for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
    uint8_t packed = pack(cases[i].r, cases[i].g, cases[i].b);
    char hex[3]; to_hex(packed, hex);
    uint8_t got = unpack(hex);
    ASSERT_EQ(got, packed, "roundtrip preserves byte");
  }
}

static void test_hex_pairs_parse_all_values(void) {
  // every byte 0xC0..0xFF survives a pack->hex->unpack round trip
  for (int v = 0xC0; v <= 0xFF; v++) {
    char hex[3]; to_hex((uint8_t)v, hex);
    ASSERT_EQ(unpack(hex), v, "byte roundtrip");
  }
}

static void test_known_palette_bytes(void) {
  // The SDK's GColor8 bytes for settings.c's defaults: white 0xFF, black
  // 0xC0, orange 0xF8, red 0xF0, duke blue 0xC2.
  char hex[3];
  to_hex(0xFF, hex); ASSERT_EQ(unpack(hex), 0xFF, "white");
  to_hex(0xC0, hex); ASSERT_EQ(unpack(hex), 0xC0, "black");
  to_hex(0xF8, hex); ASSERT_EQ(unpack(hex), 0xF8, "orange");
  to_hex(0xF0, hex); ASSERT_EQ(unpack(hex), 0xF0, "red");
  to_hex(0xC2, hex); ASSERT_EQ(unpack(hex), 0xC2, "duke blue");
  ASSERT_EQ(GColorOrange.argb, 0xF8, "mock orange matches the SDK");
  ASSERT_EQ(GColorRed.argb, 0xF0, "mock red matches the SDK");
  ASSERT_EQ(GColorDukeBlue.argb, 0xC2, "mock duke blue matches the SDK");
}

int main(void) {
  RUN(test_color_roundtrip);
  RUN(test_hex_pairs_parse_all_values);
  RUN(test_known_palette_bytes);
  TEST_SUMMARY();
}
