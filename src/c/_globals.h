#pragma once
#include <pebble.h>

/*
 * Segment is an Emery-only watchface.
 *
 * Every rectangle below is in the 200x228 screen coordinate system unless it
 * is explicitly documented as a child-layer rectangle. There is no legacy
 * display scaling or platform dispatch in the layout.
 */
#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 228
#define ANIM_DURATION 700

typedef enum {UP, DOWN, LEFT, RIGHT} direction_t;

#define FULLSCREEN GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT)

// BACKGROUND
// The panel sits between the top and bottom decoration rules, leaving a
// deliberate black breathing band around the central display.
#define BACKGROUND_PANEL GRect(0, 47, SCREEN_WIDTH, 142)
#define BACKGROUND_PANEL_OUTER GRect(0, 0, SCREEN_WIDTH, 142)
#define BACKGROUND_PANEL_MIDDLE GRect(3, 3, 194, 136)
#define BACKGROUND_PANEL_INNER GRect(6, 6, 188, 130)
#define BACKGROUND_PANEL_RADIUS 11

// PANEL
// The inner display box (the bg3 fill) in screen coordinates. The top strip
// and the health rows anchor to its edges, so the air between an item and the
// outline is a stated constant instead of an accident of a number measured
// from the screen edge.
#define PANEL_INNER_LEFT 6
#define PANEL_INNER_TOP 53
#define PANEL_INNER_RIGHT 194   // exclusive: 193 is the last lit column
#define PANEL_INNER_BOTTOM 183  // exclusive: 182 is the last lit row

// TOP STRIP
// The top strip holds the health column on the left and one right-anchored
// cluster on the right, laid out right-to-left from the panel edge:
//
//   [badge] gap [100%] gap [battery icon] gap | panel outline
//
// battery.c owns that arithmetic (battery_cluster_slot / battery_top_reserve)
// so the reserve health.c bounds its text with can never drift from what is
// actually drawn. The rects below are the nominal ones for the default case
// (percent + icon, badge hidden); position of the percent and the badge is
// derived at runtime.
#define TOP_STRIP_EDGE_GAP 4   // cluster -> panel outline
#define TOP_STRIP_ITEM_GAP 4   // between cluster members
// The strip starts a little below the panel's inner top edge: its icon column
// sits at the panel's left edge, where the outline's corner radius cuts in, so
// at PANEL_INNER_TOP the foot/zzz ink rode over the curve.
#define TOP_STRIP_INSET 3
#define TOP_STRIP_Y (PANEL_INNER_TOP + TOP_STRIP_INSET)

// BATTERY (right-most cluster member when shown)
#define BATTERY_LAYER GRect(167, TOP_STRIP_Y + 1, 23, 13)
#define BATTERY_ICON GRect(0, 0, 22, 13)
#define BATTERY_ICON_TERMINAL GRect(21, 3, 2, 7)
#define BATTERY_PERCENT GRect(111, TOP_STRIP_Y - 2, 52, 22)  // right-aligned
#define BATTERY_PERCENT_W 52
#define BATTERY_PERCENT_INK_W 33 // "100%" at Lucida 14: 32px advance + % overhang
#define PHONE_BATT_BAR GRect(167, TOP_STRIP_Y + 18, 22, 3)

// HEALTH
// Two stacked rows in the panel's left column, so a long step count can never
// reach the heart rate:
//
//   row 1: [steps/zzz icon] [steps or sleep]  — beside the battery cluster
//   row 2: [heart icon] [BPM]                 — beside the date
//
// health_layout_row() re-bounds row 1's text box against whatever the cluster
// occupies (the text is left-aligned, so it grows into that box and is clipped
// by it). Row 2 is left-anchored and needs no re-anchoring.
#define HEALTH_LEFT (PANEL_INNER_LEFT + 4)
#define HEALTH_TEXT_X 21     // local: the icon column occupies 0..18
#define HEALTH_TEXT_W 163    // nominal: widest case, nothing drawn on the right
// Row 1 shares the top strip's band, which ends where row 2 begins.
#define HEALTH_LAYER GRect(HEALTH_LEFT, TOP_STRIP_Y, PANEL_INNER_RIGHT - HEALTH_LEFT, 21)
#define HEALTH_TEXT_LAYER GRect(HEALTH_TEXT_X, 0, HEALTH_TEXT_W, 21)
#define HEALTH_BPM_ROW GRect(HEALTH_LEFT, 77, PANEL_INNER_RIGHT - HEALTH_LEFT, 18)
#define HEALTH_BPM_ICON GRect(4, 1, 14, 14)   // centred in row 1's icon column
#define HEALTH_BPM_TEXT GRect(HEALTH_TEXT_X, 0, 20, 18)
#define HEALTH_STEP_MIN 400

// BLUETOOTH (left-most cluster member when shown)
#define BLUETOOTH_BADGE_W 18
#define BLUETOOTH_BADGE_INK_W 15 // circle centre (7,7) radius 7 -> ink 0..14
#define BLUETOOTH_LAYER GRect(111, TOP_STRIP_Y, BLUETOOTH_BADGE_W, BLUETOOTH_BADGE_W)
#define BLUETOOTH_ICON_CIRCLE GRect(0, 0, 18, 18)
#define BLUETOOTH_ICON_SYMBOL GRect(3, 2, 10, 13)

// TIMEDIGITS
// TIMEDIGITS_CENTER is the absolute screen coordinate space. Its children
// therefore use screen coordinates too, avoiding the old double-origin bug.
#define TIMEDIGITS_CENTER FULLSCREEN
#define TIMEDIGITS_DATE GRect(42, 77, 150, 18)

// Normal time: the 99px Emery font has approximately 50px digit advances.
// The separator box overlaps the neighbouring boxes intentionally because
// the colon glyph is narrower than a digit and is right-aligned in its box.
#define TIMEDIGITS_DIGIT1 4
#define TIMEDIGITS_DIGIT2 47
#define TIMEDIGITS_DIGIT3 103
#define TIMEDIGITS_DIGIT4 146
#define TIMEDIGITS_SEPARATOR 61
// The clock sits at the same height whether or not seconds are shown: its ink
// runs 95..157, which clears the date row above and leaves the panel's bottom
// row for the weather readout and the seconds. (It used to drop to 115..177
// when seconds were off, which is the row the weather now occupies.)
#define TIMEDIGITS_OFFSET_TOP 59
#define TIMEDIGITS_WIDTH 50
#define TIMEDIGITS_HEIGHT 115

// Seconds mode: the clock keeps the 99px font, the normal digit boxes and the
// normal full-width layout — it only moves up — and the seconds pair gets its
// own row underneath, centred on the panel.
//
// The old design shrank the clock to the 78px font and shifted the readout
// 4px left so the pair could sit beside it. That put the hour-tens digit's box
// over the panel outline, where its background-coloured shadow layer showed
// through as a ghost digit straddling the border, and the pair itself was left
// flush on the right edge.
//
// Measured ink offsets (emulator, frame_top -> ink):
//   99px digits: +36 .. +98      26px seconds: +9 .. +25
// so the pair's row is derived from the clock's ink bottom rather than tuned by
// hand: the clock's ink ends at 157 and the pair's runs 161..177, leaving rows
// clear above the panel's inner bottom edge.
#define TIMEDIGITS_DIGIT_INK_TOP 36
#define TIMEDIGITS_DIGIT_INK_BOTTOM 98
#define TIMEDIGITS_SECONDS_PAIR_INK_TOP 9
#define TIMEDIGITS_SECONDS_PAIR_INK_BOTTOM 25

// Pair: two 14px boxes 13px apart (they overlap by a pixel, as the old
// side-by-side pair did), right-aligned inside the panel with the same edge gap
// the rest of the face keeps — the weather readout now owns the left of this
// row, so the seconds moved off centre to make room for it.
#define TIMEDIGITS_SECONDS_PAIR_GAP 3  // air between the clock's ink and theirs
#define TIMEDIGITS_SECONDS_PAIR_STEP 13
#define TIMEDIGITS_SECONDS_SMALL_WIDTH 14
#define TIMEDIGITS_SECONDS_SMALL_HEIGHT 32
#define TIMEDIGITS_SECONDS_PAIR_X (PANEL_INNER_RIGHT - TOP_STRIP_EDGE_GAP \
                                   - TIMEDIGITS_SECONDS_PAIR_STEP \
                                   - TIMEDIGITS_SECONDS_SMALL_WIDTH)
#define TIMEDIGITS_SECONDS_PAIR_Y \
  (TIMEDIGITS_OFFSET_TOP + TIMEDIGITS_DIGIT_INK_BOTTOM + 1 \
   + TIMEDIGITS_SECONDS_PAIR_GAP - TIMEDIGITS_SECONDS_PAIR_INK_TOP)

// DECORATIONS
#define DECORATIONS_LINE_TOP_START GPoint(0, 26)
#define DECORATIONS_LINE_TOP_END GPoint(SCREEN_WIDTH, 26)
#define DECORATIONS_LINE_BOTTOM_START GPoint(0, 208)
#define DECORATIONS_LINE_BOTTOM_END GPoint(SCREEN_WIDTH, 208)

// Button labels carry user text (LABEL_MAX glyphs, 8px each in Lucida 14), so
// the boxes are sized for the longest label the setting accepts rather than for
// the stock wording. BACK grows right from its icon; PREV and NEXT grow left,
// towards the panel edge, and are right-aligned so the text stays beside its
// arrow whatever its length. None of them reach the panel or the button icons.
#define DECORATIONS_BUTTON_BACK_LABEL GRect(18, 29, 64, 14)
#define DECORATIONS_BUTTON_NEXT_LABEL GRect(118, 190, 64, 14)
#define DECORATIONS_BUTTON_PREV_LABEL GRect(118, 29, 64, 14)
#define DECORATIONS_BUTTON_BACK_ICON GRect(7, 33, 7, 8)
#define DECORATIONS_BUTTON_NEXT_ICON GRect(185, 33, 7, 8)
#define DECORATIONS_BUTTON_PREV_ICON GRect(185, 195, 7, 8)

#define DECORATIONS_LOGO GRect(74, 3, 52, 15)

// The centre outline stays decorative; the temperature readout moved inside the
// panel onto the seconds' row: [icon] [now°(high°/low°)] on the left, seconds on
// the right. The icon's bottom edge and the text's baseline sit on the seconds'
// ink bottom, so the three read as one row.
#define DECORATIONS_WR_OUTER GRect(71, 206, 58, 22)
#define DECORATIONS_WEATHER_ICON GRect(8, 161, 16, 16)
#define DECORATIONS_WEATHER_TEXT GRect(26, 164, 130, 18)
