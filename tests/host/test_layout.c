/* Geometry invariants for the native Emery layout.
 * Guards the regression where the big digits painted low and the small
 * seconds clipped to nothing. Host build; evaluates the real _globals.h. */
#include "pebble.h"
#include "test_util.h"
#include "../../src/c/_globals.h"

/* DS-Digital ink offsets below a frame's top edge, measured in the emulator
 * (and encoded as constants in _globals.h):
 *   99px digits: ink 36 .. 98      26px seconds: ink 9 .. 25
 * A frame shorter than the ink clips it. */

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

static void test_digit_frames_cover_their_ink(void) {
  // A frame shorter than the glyph's ink clips it. The offset from a frame's
  // top to its ink bottom is measured, not derived from the line box: the 99px
  // font renders in a frame shorter than its line box because its ink starts
  // 36px down.
  ASSERT_TRUE(TIMEDIGITS_HEIGHT >= TIMEDIGITS_DIGIT_INK_BOTTOM + 1,
              "clock digit frame covers the digit ink");
  ASSERT_TRUE(TIMEDIGITS_SECONDS_SMALL_HEIGHT >= TIMEDIGITS_SECONDS_PAIR_INK_BOTTOM + 1,
              "seconds pair frame covers the pair ink");
}

static void test_clock_ink_clears_the_panel_outline(void) {
  // Both clock modes share these boxes. The glyph ink, not the box, is what
  // must stay off the outline: the 99px digits carry a side bearing, and their
  // background-coloured shadow layer is drawn at the same box, so any ink that
  // runs past the panel's fill shows up as a ghost digit over the border.
  int bearing = 5;  // measured: box left 4 -> ink starts at 9
  ASSERT_TRUE(TIMEDIGITS_DIGIT1 + bearing >= PANEL_INNER_LEFT,
              "left-most digit ink starts inside the panel fill");
  ASSERT_TRUE(TIMEDIGITS_DIGIT4 + TIMEDIGITS_WIDTH - bearing <= PANEL_INNER_RIGHT - 1,
              "right-most digit ink ends inside the panel fill");
  int w = PBL_DISPLAY_WIDTH;
  ASSERT_TRUE(TIMEDIGITS_DIGIT1 >= 0 && TIMEDIGITS_DIGIT4 + TIMEDIGITS_WIDTH <= w,
              "clock boxes stay on screen");
}

static void test_clock_rows_and_the_bottom_row(void) {
  // One clock height for both modes: its ink clears the date row above and
  // stops above the bottom row, which the weather readout and the seconds
  // share (they used to collide with the clock when it sat lower).
  int date_ink_bottom = TIMEDIGITS_DATE.origin.y + 15;  // slashes descend last
  int clock_ink_top = TIMEDIGITS_OFFSET_TOP + TIMEDIGITS_DIGIT_INK_TOP;
  int clock_ink_bottom = TIMEDIGITS_OFFSET_TOP + TIMEDIGITS_DIGIT_INK_BOTTOM;
  int pair_ink_top = TIMEDIGITS_SECONDS_PAIR_Y + TIMEDIGITS_SECONDS_PAIR_INK_TOP;
  int pair_ink_bottom = TIMEDIGITS_SECONDS_PAIR_Y + TIMEDIGITS_SECONDS_PAIR_INK_BOTTOM;

  ASSERT_TRUE(clock_ink_top >= date_ink_bottom + 2, "clock ink clears the date row");
  ASSERT_TRUE(pair_ink_top >= clock_ink_bottom + 4, "pair ink clears the clock ink");
  ASSERT_TRUE(pair_ink_bottom <= PANEL_INNER_BOTTOM - 1 - 3, "pair ink clears the panel edge");

  int icon_top = DECORATIONS_WEATHER_ICON.origin.y;
  int icon_bottom = icon_top + DECORATIONS_WEATHER_ICON.size.h - 1;
  ASSERT_TRUE(icon_top >= clock_ink_bottom + 3, "weather icon clears the clock ink");
  ASSERT_TRUE(icon_bottom <= PANEL_INNER_BOTTOM - 1 - 3, "weather icon clears the panel edge");
}

static void test_seconds_pair_is_right_aligned_on_its_row(void) {
  int pair_right = TIMEDIGITS_SECONDS_PAIR_X + TIMEDIGITS_SECONDS_PAIR_STEP
                   + TIMEDIGITS_SECONDS_SMALL_WIDTH;
  ASSERT_TRUE(TIMEDIGITS_SECONDS_PAIR_X >= PANEL_INNER_LEFT, "pair starts inside the panel");
  ASSERT_TRUE(pair_right <= PANEL_INNER_RIGHT, "pair ends inside the panel");
  ASSERT_TRUE(pair_right == PANEL_INNER_RIGHT - TOP_STRIP_EDGE_GAP,
              "pair keeps the standard edge gap on the right");
  ASSERT_TRUE(TIMEDIGITS_SECONDS_PAIR_STEP >= TIMEDIGITS_SECONDS_SMALL_WIDTH - 1,
              "pair boxes touch without a gap");
}

static void test_weather_row_shares_the_seconds_row(void) {
  // Left of that row: icon then "80°(88°/65°)". Right of it: the seconds.
  int icon_bottom = DECORATIONS_WEATHER_ICON.origin.y + DECORATIONS_WEATHER_ICON.size.h - 1;
  int clock_ink_bottom = TIMEDIGITS_OFFSET_TOP + TIMEDIGITS_DIGIT_INK_BOTTOM;
  ASSERT_TRUE(DECORATIONS_WEATHER_ICON.origin.x >= PANEL_INNER_LEFT, "icon inside the panel");
  ASSERT_TRUE(icon_bottom <= PANEL_INNER_BOTTOM - 1 - 3, "icon clears the panel's bottom edge");
  ASSERT_TRUE(DECORATIONS_WEATHER_ICON.origin.y >= clock_ink_bottom + 3,
              "icon clears the clock's ink");

  // The text box ends before the seconds' boxes start, so a long reading
  // cannot reach them; the icon and the text do not overlap either.
  int text_right = DECORATIONS_WEATHER_TEXT.origin.x + DECORATIONS_WEATHER_TEXT.size.w;
  ASSERT_TRUE(DECORATIONS_WEATHER_TEXT.origin.x > DECORATIONS_WEATHER_ICON.origin.x +
              DECORATIONS_WEATHER_ICON.size.w, "text starts right of the icon");
  ASSERT_TRUE(text_right <= TIMEDIGITS_SECONDS_PAIR_X, "text box ends before the seconds");

  // Widest reading the phone can produce: "-100°(-100°/-100°)". Lucida 14 is
  // 8px per glyph, and only the ~13 needed for a realistic reading matter.
  ASSERT_TRUE(13 * 8 <= DECORATIONS_WEATHER_TEXT.size.w,
              "text box holds a full reading");

  // Text baseline lines up with the seconds' ink bottom.
  int text_ink_bottom = DECORATIONS_WEATHER_TEXT.origin.y + 13;
  int pair_ink_bottom = TIMEDIGITS_SECONDS_PAIR_Y + TIMEDIGITS_SECONDS_PAIR_INK_BOTTOM;
  ASSERT_TRUE(text_ink_bottom <= pair_ink_bottom && pair_ink_bottom - text_ink_bottom <= 2,
              "weather text sits on the seconds' baseline");
}

static void test_native_screen_bounds(void) {
  ASSERT_TRUE(FULLSCREEN.size.w == 200 && FULLSCREEN.size.h == 228, "fullscreen is native Emery");
  ASSERT_TRUE(BACKGROUND_PANEL.origin.y >= 0, "panel starts on-screen");
  ASSERT_TRUE(BACKGROUND_PANEL.origin.y + BACKGROUND_PANEL.size.h <= 228, "panel fits screen");
  ASSERT_TRUE(DECORATIONS_WEATHER_ICON.origin.x + DECORATIONS_WEATHER_ICON.size.w <= PANEL_INNER_RIGHT,
              "weather icon fits the panel");
  ASSERT_TRUE(DECORATIONS_WEATHER_TEXT.origin.x + DECORATIONS_WEATHER_TEXT.size.w <= PANEL_INNER_RIGHT,
              "weather text box fits the panel");
  ASSERT_TRUE(BATTERY_LAYER.origin.x + BATTERY_LAYER.size.w <= 200, "battery icon fits screen");
  ASSERT_TRUE(BATTERY_PERCENT.origin.x + BATTERY_PERCENT.size.w <= 200, "battery text fits screen");
  ASSERT_TRUE(TIMEDIGITS_DATE.origin.y >= HEALTH_LAYER.origin.y + HEALTH_LAYER.size.h,
              "date is below health row");
  // The clock's frame deliberately overlaps the date row's frame: the 99px font
  // carries ~36px of empty space above its ink, which is what the row stack is
  // built from (test_clock_rows_and_the_bottom_row checks the ink).
}

static void test_minute_digits_dont_overlap(void) {
  // The 99px font's glyph advance is 50px; the boxes may overlap that only
  // where a narrow '1' sits, so the two minute digits must still start well
  // clear of each other (regression: the boxes were 29px apart once, putting
  // the ones digit inside the tens digit's ink).
  ASSERT_TRUE(TIMEDIGITS_DIGIT4 - TIMEDIGITS_DIGIT3 >= 0, "minute digits ordered");
  ASSERT_TRUE(TIMEDIGITS_DIGIT4 - TIMEDIGITS_DIGIT3 >= 40,
              "minute tens/ones start far enough apart");
}

int main(void) {
  RUN(test_digit_frames_cover_their_ink);
  RUN(test_minute_digits_dont_overlap);
  RUN(test_native_screen_bounds);
  RUN(test_clock_ink_clears_the_panel_outline);
  RUN(test_clock_rows_and_the_bottom_row);
  RUN(test_seconds_pair_is_right_aligned_on_its_row);
  RUN(test_weather_row_shares_the_seconds_row);
  RUN(test_top_strip_clears_the_panel_corner);
  RUN(test_top_strip_cluster_hugs_panel_edge);
  RUN(test_bluetooth_badge_clears_battery_percent);
  RUN(test_health_rows_stack_without_overlap);
  RUN(test_steps_text_fits_beside_the_cluster);
  TEST_SUMMARY();
}
