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
#define TOP_STRIP_Y 53         // badge band top

// BATTERY (right-most cluster member when shown)
#define BATTERY_LAYER GRect(167, 54, 23, 13)
#define BATTERY_ICON GRect(0, 0, 22, 13)
#define BATTERY_ICON_TERMINAL GRect(21, 3, 2, 7)
#define BATTERY_PERCENT GRect(111, 51, 52, 22)  // right-aligned text box
#define BATTERY_PERCENT_W 52
#define BATTERY_PERCENT_INK_W 33 // "100%" at Lucida 14: 32px advance + % overhang
#define PHONE_BATT_BAR GRect(167, 71, 22, 3)

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
#define HEALTH_LAYER GRect(HEALTH_LEFT, PANEL_INNER_TOP, PANEL_INNER_RIGHT - HEALTH_LEFT, 24)
#define HEALTH_TEXT_LAYER GRect(HEALTH_TEXT_X, 0, HEALTH_TEXT_W, 22)
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
#define TIMEDIGITS_OFFSET_TOP 79
#define TIMEDIGITS_WIDTH 50
#define TIMEDIGITS_HEIGHT 115

// Seconds mode: big minute/hour digits use the 78px font, with the small
// seconds pair placed below and right-aligned inside the display.
//
// DS-Digital metrics (TTF-measured): both sized glyphs bottom-anchor to the
// font ascent — 78px font: ink bottom = frame_top + 80; 31px font: ink
// bottom = frame_top + 32 (line boxes 95/38, cap offsets 14/12).
// TIMEDIGITS_SECONDS_SMALL_OFFSET_TOP = 127 makes the small seconds' ink
// bottom 127+32 = 159 = the big digits' 79+80, so both rows sit on the same
// baseline. The previous 131 dropped the seconds 4px below the big digits.
//
// Horizontal: the 78px glyph advance is 40px, so adjacent minute digits
// must start >= 40px apart or their ink overlaps (a narrow tens '1' then
// sat inside the ones digit). DIGIT4 = DIGIT3 + 40; the small seconds keep
// the same box-touch spacing, ending 2px inside the right screen edge.
#define TIMEDIGITS_SECONDS_DIGIT1 4
#define TIMEDIGITS_SECONDS_DIGIT2 39
#define TIMEDIGITS_SECONDS_DIGIT3 86
#define TIMEDIGITS_SECONDS_DIGIT4 126
#define TIMEDIGITS_SECONDS_DIGIT5 166
#define TIMEDIGITS_SECONDS_DIGIT6 181
#define TIMEDIGITS_SECONDS_SEPARATOR 50
#define TIMEDIGITS_SECONDS_OFFSET_TOP 79
#define TIMEDIGITS_SECONDS_SMALL_OFFSET_TOP 127
#define TIMEDIGITS_SECONDS_WIDTH 40
#define TIMEDIGITS_SECONDS_SMALL_WIDTH 17
#define TIMEDIGITS_SECONDS_HEIGHT 95
#define TIMEDIGITS_SECONDS_SMALL_HEIGHT 38

// DECORATIONS
#define DECORATIONS_LINE_TOP_START GPoint(0, 26)
#define DECORATIONS_LINE_TOP_END GPoint(SCREEN_WIDTH, 26)
#define DECORATIONS_LINE_BOTTOM_START GPoint(0, 208)
#define DECORATIONS_LINE_BOTTOM_END GPoint(SCREEN_WIDTH, 208)

#define DECORATIONS_BUTTON_BACK_LABEL GRect(18, 29, 49, 14)
#define DECORATIONS_BUTTON_NEXT_LABEL GRect(147, 190, 35, 14)
#define DECORATIONS_BUTTON_PREV_LABEL GRect(147, 29, 35, 14)
#define DECORATIONS_BUTTON_BACK_ICON GRect(7, 33, 7, 8)
#define DECORATIONS_BUTTON_NEXT_ICON GRect(185, 33, 7, 8)
#define DECORATIONS_BUTTON_PREV_ICON GRect(185, 195, 7, 8)

#define DECORATIONS_LOGO GRect(74, 3, 52, 15)

// Weather readouts occupy the bottom strip and end exactly at the 228px
// screen edge. The center outline remains decorative.
#define DECORATIONS_WR_OUTER GRect(71, 206, 58, 22)
#define DECORATIONS_TEMP_HI GRect(0, 211, 64, 17)
#define DECORATIONS_TEMP_LO GRect(136, 211, 64, 17)
