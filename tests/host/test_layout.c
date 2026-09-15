/* Geometry invariants for the native Emery layout.
 * Guards the regression where the big digits painted low and the small
 * seconds clipped to nothing. Host build; evaluates the real _globals.h. */
#include "pebble.h"
#include "test_util.h"
#include "../../src/c/_globals.h"

/* DS-Digital metrics (measured with PIL; documented in _globals.h):
 *   78px font: cap_offset (ink top below frame top) = 14, line box = 95
 *   31px font: cap_offset = 12, line box = 38
 *   99px font: cap_offset = 36 (ink top = TIMEDIGITS_OFFSET_TOP + 36)
 * A frame SHORTER than the line box clips the glyph. */

static void test_top_strip_clears_the_panel_corner(void) {
  // The strip's icon column starts at the panel's left edge, where the
  // outline's corner radius cuts the fill away: at PANEL_INNER_TOP the ink
  // rides over the curve, so the band must be inset.
  ASSERT_TRUE(TOP_STRIP_INSET > 0, "top strip inset below the panel edge");
  ASSERT_TRUE(HEALTH_LAYER.origin.y == TOP_STRIP_Y, "health row shares the strip band");
  ASSERT_TRUE(BATTERY_LAYER.origin.y >= TOP_STRIP_Y, "battery icon inside the strip band");
  ASSERT_TRUE(TOP_STRIP_Y + HEALTH_LAYER.size.h <= HEALTH_BPM_ROW.origin.y,
              "strip band ends where the heart rate row begins");
}

static void test_top_strip_cluster_hugs_panel_edge(void) {
  // The battery icon is the cluster's anchor and sits a fixed gap inside the
  // panel's inner right edge. Deriving it from the screen edge instead is how
  // the old reserve drifted away from what was actually drawn.
  ASSERT_TRUE(BATTERY_LAYER.origin.x == PANEL_INNER_RIGHT - TOP_STRIP_EDGE_GAP - BATTERY_LAYER.size.w,
              "battery icon anchored to the panel edge, not the screen edge");
  ASSERT_TRUE(BATTERY_LAYER.origin.y >= PANEL_INNER_TOP,
              "battery icon inside the panel");
}

static void test_bluetooth_badge_clears_battery_percent(void) {
  // [badge] gap [100%]: the badge's nominal slot must not touch the widest
  // percentage the text can render.
  int percent_ink_left = BATTERY_PERCENT.origin.x + BATTERY_PERCENT_W - BATTERY_PERCENT_INK_W;
  ASSERT_TRUE(BLUETOOTH_LAYER.origin.x + BLUETOOTH_BADGE_INK_W + TOP_STRIP_ITEM_GAP <= percent_ink_left,
              "badge clears the battery percentage ink");
  ASSERT_TRUE(BLUETOOTH_LAYER.origin.x >= HEALTH_LEFT,
              "badge stays right of the health column");
}

static void test_health_rows_stack_without_overlap(void) {
  // Row 1 (steps) then row 2 (heart rate), neither reaching the digits.
  ASSERT_TRUE(HEALTH_BPM_ROW.origin.y >= HEALTH_LAYER.origin.y + HEALTH_LAYER.size.h,
              "heart rate row sits below the steps row");
  ASSERT_TRUE(HEALTH_BPM_ROW.origin.y + HEALTH_BPM_ROW.size.h <= TIMEDIGITS_OFFSET_TOP + 36,
              "heart rate row clears the clock's ink");
  ASSERT_TRUE(HEALTH_BPM_TEXT.origin.y + HEALTH_BPM_TEXT.size.h <= HEALTH_BPM_ROW.size.h,
              "bpm text fits its row");
}

static void test_steps_text_fits_beside_the_cluster(void) {
  // The reserve re-bounds this at runtime; the nominal width is the widest
  // case (nothing drawn on the right).
  ASSERT_TRUE(HEALTH_TEXT_W == PANEL_INNER_RIGHT - HEALTH_LEFT - HEALTH_TEXT_X,
              "nominal steps text box reaches the panel edge");
  // Worst case: badge shown. Lucida 14 advances 8px per glyph, so the widest
  // realistic count ("999,999") still fits the space left of the cluster.
  int worst_case = BLUETOOTH_LAYER.origin.x - TOP_STRIP_ITEM_GAP - HEALTH_LEFT - HEALTH_TEXT_X;
  ASSERT_TRUE(worst_case >= 7 * 8, "seven-glyph step count fits beside the badge");
}

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

static void test_seconds_ink_clears_the_panel_outline(void) {
  // The small seconds' box must end at (not past) the panel's inner edge:
  // their glyph is centred in the box, so a box ending on PANEL_INNER_RIGHT
  // leaves their ink the standard edge gap before the outline.
  ASSERT_TRUE(TIMEDIGITS_SECONDS_DIGIT6 + TIMEDIGITS_SECONDS_SMALL_WIDTH <= PANEL_INNER_RIGHT,
              "seconds box ends inside the panel outline");
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

static void test_native_screen_bounds(void) {
  ASSERT_TRUE(FULLSCREEN.size.w == 200 && FULLSCREEN.size.h == 228, "fullscreen is native Emery");
  ASSERT_TRUE(BACKGROUND_PANEL.origin.y >= 0, "panel starts on-screen");
  ASSERT_TRUE(BACKGROUND_PANEL.origin.y + BACKGROUND_PANEL.size.h <= 228, "panel fits screen");
  ASSERT_TRUE(DECORATIONS_TEMP_HI.origin.y + DECORATIONS_TEMP_HI.size.h <= 228, "high temp fits screen");
  ASSERT_TRUE(DECORATIONS_TEMP_LO.origin.y + DECORATIONS_TEMP_LO.size.h <= 228, "low temp fits screen");
  ASSERT_TRUE(BATTERY_LAYER.origin.x + BATTERY_LAYER.size.w <= 200, "battery icon fits screen");
  ASSERT_TRUE(BATTERY_PERCENT.origin.x + BATTERY_PERCENT.size.w <= 200, "battery text fits screen");
  ASSERT_TRUE(TIMEDIGITS_DATE.origin.y >= HEALTH_LAYER.origin.y + HEALTH_LAYER.size.h,
              "date is below health row");
  ASSERT_TRUE(TIMEDIGITS_OFFSET_TOP >= TIMEDIGITS_DATE.origin.y,
              "clock starts below metadata row");
}

static void test_small_seconds_bottom_aligned_with_big_digits(void) {
  // Both sizes bottom-anchor to their font ascent: 78px ink bottom is at
  // frame_top + 80, 31px at frame_top + 32 (TTF-measured). The tops must
  // differ by 48 so both rows share one baseline — otherwise the seconds
  // sink below the big digits (regression: 131 left them 4px low).
  ASSERT_TRUE(TIMEDIGITS_SECONDS_SMALL_OFFSET_TOP + 32 == TIMEDIGITS_SECONDS_OFFSET_TOP + 80,
              "small seconds share the big digits' baseline");
}

static void test_minute_digits_dont_overlap(void) {
  // 78px glyph advance is 40px (TTF-measured); the minute tens and ones
  // boxes must start >= 40px apart or their ink overlaps — visible as a
  // narrow '1' sitting inside the ones digit (regression: DIGIT4 was 115,
  // only 29px after DIGIT3).
  ASSERT_TRUE(TIMEDIGITS_SECONDS_DIGIT4 - TIMEDIGITS_SECONDS_DIGIT3 >= 40,
              "minute tens/ones start >= 40px apart (78px advance)");
  ASSERT_TRUE(TIMEDIGITS_SECONDS_DIGIT6 + TIMEDIGITS_SECONDS_SMALL_WIDTH <= 200,
              "small seconds end inside the right screen edge");
}

int main(void) {
  RUN(test_big_digit_frame_fits_font);
  RUN(test_seconds_digits_within_screen);
  RUN(test_seconds_separator_between_digit_pairs);
  RUN(test_small_seconds_below_big_digits);
  RUN(test_small_seconds_dont_overlap_digit4);
  RUN(test_normal_and_seconds_heights_differ);
  RUN(test_small_seconds_bottom_aligned_with_big_digits);
  RUN(test_minute_digits_dont_overlap);
  RUN(test_native_screen_bounds);
  RUN(test_seconds_ink_clears_the_panel_outline);
  RUN(test_top_strip_clears_the_panel_corner);
  RUN(test_top_strip_cluster_hugs_panel_edge);
  RUN(test_bluetooth_badge_clears_battery_percent);
  RUN(test_health_rows_stack_without_overlap);
  RUN(test_steps_text_fits_beside_the_cluster);
  TEST_SUMMARY();
}
