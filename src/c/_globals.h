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

// BATTERY
#define BATTERY_LAYER GRect(167, 54, 23, 13)
#define BATTERY_ICON GRect(0, 0, 22, 13)
#define BATTERY_ICON_TERMINAL GRect(21, 3, 2, 7)
#define BATTERY_PERCENT GRect(112, 51, 52, 22)
#define PHONE_BATT_BAR GRect(167, 71, 22, 3)

// HEALTH
// These frames are child rectangles inside HEALTH_LAYER. The health row is
// re-anchored horizontally by health_layout_row() when BT/battery settings
// change.
#define HEALTH_LAYER GRect(25, 53, 115, 24)
#define HEALTH_TEXT_LAYER GRect(21, 0, 70, 22)
#define HEALTH_BPM_ICON GRect(68, 0, 14, 14)
#define HEALTH_BPM_TEXT GRect(85, 0, 20, 22)
#define HEALTH_STEP_MIN 400

// BLUETOOTH
#define BLUETOOTH_LAYER GRect(6, 53, 18, 18)
#define BLUETOOTH_ICON_CIRCLE GRect(0, 0, 18, 18)
#define BLUETOOTH_ICON_SYMBOL GRect(3, 2, 10, 13)

// TIMEDIGITS
// TIMEDIGITS_CENTER is the absolute screen coordinate space. Its children
// therefore use screen coordinates too, avoiding the old double-origin bug.
#define TIMEDIGITS_CENTER FULLSCREEN
#define TIMEDIGITS_DATE GRect(42, 77, 150, 18)
#define TIMEDIGITS_AMPM GRect(8, 77, 30, 18)

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
#define TIMEDIGITS_SECONDS_DIGIT1 4
#define TIMEDIGITS_SECONDS_DIGIT2 39
#define TIMEDIGITS_SECONDS_DIGIT3 86
#define TIMEDIGITS_SECONDS_DIGIT4 115
#define TIMEDIGITS_SECONDS_DIGIT5 155
#define TIMEDIGITS_SECONDS_DIGIT6 172
#define TIMEDIGITS_SECONDS_SEPARATOR 50
#define TIMEDIGITS_SECONDS_OFFSET_TOP 79
#define TIMEDIGITS_SECONDS_SMALL_OFFSET_TOP 131
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
