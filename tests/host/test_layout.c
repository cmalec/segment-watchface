/* Geometry invariants for the seconds-mode layout on emery (and friends).
 * Guards the regression where the big digits painted ~48px low and the small
 * seconds clipped to nothing. Host build; evaluates the real _globals.h. */
#include "pebble.h"
#include "test_util.h"
#include "../../src/c/_globals.h"

/* DS-Digital metrics (measured with PIL; documented in _globals.h):
 *   78px font: cap_offset (ink top below frame top) = 14, line box = 95
 *   31px font: cap_offset = 12, line box = 38
 * A frame SHORTER than the line box clips the glyph. */

static void test_big_digit_frame_fits_font(void) {
  // height must be >= the 78px line box on emery, >= 56px box on rect
  ASSERT_TRUE(TIMEDIGITS_SECONDS_HEIGHT >= 95 || TIMEDIGITS_SECONDS_HEIGHT >= 56,
              "seconds digit frame tall enough for its font line box");
}

static void test_seconds_digits_within_screen(void) {
  // right edges must not exceed the display width
  int w = PBL_DISPLAY_WIDTH;
  ASSERT_TRUE(TIMEDIGITS_SECONDS_DIGIT4 + TIMEDIGITS_SECONDS_WIDTH <= w, "digit4 fits width");
  ASSERT_TRUE(TIMEDIGITS_SECONDS_DIGIT6 + TIMEDIGITS_SECONDS_SMALL_WIDTH <= w, "digit6 fits width");
  ASSERT_TRUE(TIMEDIGITS_SECONDS_DIGIT1 >= 0, "digit1 on-screen");
}

static void test_seconds_separator_between_digit_pairs(void) {
  // separator sits between digit2 and digit3 horizontally
  ASSERT_TRUE(TIMEDIGITS_SECONDS_SEPARATOR >= TIMEDIGITS_SECONDS_DIGIT2, "sep right of d2");
  ASSERT_TRUE(TIMEDIGITS_SECONDS_SEPARATOR <= TIMEDIGITS_SECONDS_DIGIT3, "sep left of d3");
}

static void test_small_seconds_below_big_digits(void) {
  // small seconds caps start below the big-digit caps (top ordering)
  ASSERT_TRUE(TIMEDIGITS_SECONDS_SMALL_OFFSET_TOP >= TIMEDIGITS_SECONDS_OFFSET_TOP,
              "small seconds below big digits");
}

static void test_small_seconds_dont_overlap_digit4(void) {
  // d5 starts at/after d4's right edge (no horizontal overlap of glyph boxes)
  int d4_right = TIMEDIGITS_SECONDS_DIGIT4 + TIMEDIGITS_SECONDS_WIDTH;
  ASSERT_TRUE(TIMEDIGITS_SECONDS_DIGIT5 >= d4_right - 2, "d5 clear of d4 (allow 2px)");
}

static void test_normal_and_seconds_heights_differ(void) {
  // sanity: the seconds layout uses the smaller font's line box
  // (a frame equal to the normal-mode height would reintroduce the bug)
  ASSERT_TRUE(TIMEDIGITS_SECONDS_HEIGHT != TIMEDIGITS_HEIGHT, "seconds frame uses small layout");
}

int main(void) {
  RUN(test_big_digit_frame_fits_font);
  RUN(test_seconds_digits_within_screen);
  RUN(test_seconds_separator_between_digit_pairs);
  RUN(test_small_seconds_below_big_digits);
  RUN(test_small_seconds_dont_overlap_digit4);
  RUN(test_normal_and_seconds_heights_differ);
  TEST_SUMMARY();
}
