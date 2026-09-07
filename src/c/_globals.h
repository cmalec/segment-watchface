#pragma once
#include <pebble.h>

/*
 * Layout geometry.
 *
 * Basalt (144x168) is the reference design. Emery (Pebble Time 2, 200x228)
 * is almost exactly basalt scaled by 1.389 (168 * 1.389 = 233), so its
 * geometry below is the basalt layout uniformly scaled — this preserves the
 * original composition on the larger display.
 *
 * SCREEN_ELSE picks emery / rect / round values; it exists because
 * PBL_IF_RECT_ELSE would hand emery the unscaled 144x168 branch.
 */
#if defined(PBL_PLATFORM_EMERY)
#define SCREEN_ELSE(EMERY_VAL, RECT_VAL, ROUND_VAL) (EMERY_VAL)
#elif defined(PBL_RECT)
#define SCREEN_ELSE(EMERY_VAL, RECT_VAL, ROUND_VAL) (RECT_VAL)
#else
#define SCREEN_ELSE(EMERY_VAL, RECT_VAL, ROUND_VAL) (ROUND_VAL)
#endif

#define ANIM_DURATION 400

typedef enum {UP, DOWN, LEFT, RIGHT} direction_t;

#define FULLSCREEN SCREEN_ELSE(GRect(0, 0, 200, 228), \
                               PBL_IF_RECT_ELSE(GRect(0, 0, 144, 168), GRect(0, 0, 180, 180)), \
                               GRect(0, 0, 180, 180))

// BACKGROUND
#define BACKGROUND_PANEL SCREEN_ELSE(GRect(0, 47, 200, 142), \
                                     PBL_IF_RECT_ELSE(GRect(0, 34, 144, 102), GRect(0, 0, 180, 180)), \
                                     GRect(0, 0, 180, 180))
#define BACKGROUND_PANEL_OUTER SCREEN_ELSE(GRect(0, 0, 200, 142), \
                                           PBL_IF_RECT_ELSE(GRect(0, 0, 144, 102), GRect(0, 0, 180, 180)), \
                                           GRect(0, 0, 180, 180))
#define BACKGROUND_PANEL_MIDDLE SCREEN_ELSE(GRect(3, 3, 194, 136), \
                                            PBL_IF_RECT_ELSE(GRect(2, 2, 140, 98), GRect(2, 2, 176, 176)), \
                                            GRect(2, 2, 176, 176))
#define BACKGROUND_PANEL_INNER SCREEN_ELSE(GRect(6, 6, 188, 130), \
                                           PBL_IF_RECT_ELSE(GRect(4, 4, 136, 94), GRect(4, 4, 172, 172)), \
                                           GRect(4, 4, 172, 172))
#define BACKGROUND_PANEL_RADIUS SCREEN_ELSE(11, 8, 8)

//BATTERY
#define BATTERY_LAYER SCREEN_ELSE(GRect(167, 60, 23, 13), \
                                  PBL_IF_RECT_ELSE(GRect(120, 43, 17, 9), GRect(128, 51, 17, 9)), \
                                  GRect(128, 51, 17, 9))
#define BATTERY_ICON SCREEN_ELSE(GRect(0,0,22,13), GRect(0,0,16,9), GRect(0,0,16,9))
#define BATTERY_ICON_TERMINAL SCREEN_ELSE(GRect(21,3,2,7), GRect(15,2,2,5), GRect(15,2,2,5))
#define BATTERY_PERCENT SCREEN_ELSE(GRect(115, 57, 49, 22), \
                                    PBL_IF_RECT_ELSE(GRect(83, 41, 35, 16), GRect(128, 51, 35, 16)), \
                                    GRect(128, 51, 35, 16))

// Phone battery: thin bar directly under the watch battery icon, filled
// proportionally to the phone's charge (sent from the phone via PebbleKit
// JS Battery Status API, where available). Hidden until data arrives.
#define PHONE_BATT_BAR SCREEN_ELSE(GRect(167, 77, 22, 3), PBL_IF_RECT_ELSE(GRect(120, 54, 16, 2), GRect(128, 62, 16, 2)), GRect(128, 62, 16, 2))

//HEALTH
#define HEALTH_LAYER SCREEN_ELSE(GRect(25, 57, 115, 22), \
                                 PBL_IF_RECT_ELSE(GRect(25, 41, 70, 16), GRect(71, 123, 70, 16)), \
                                 GRect(71, 123, 70, 16))
#define HEALTH_TEXT_LAYER SCREEN_ELSE(GRect(21, 0, 70, 22), GRect(15, 0, 67, 16), GRect(15, 0, 67, 16))
#define HEALTH_BPM_ICON SCREEN_ELSE(GRect(68, 0, 14, 14), GRect(0, 0, 0, 0), GRect(0, 0, 0, 0))
#define HEALTH_BPM_TEXT SCREEN_ELSE(GRect(85, 0, 20, 22), GRect(0, 0, 0, 0), GRect(0, 0, 0, 0))
#define HEALTH_STEP_MIN 400

//BLUETOOTH
#define BLUETOOTH_LAYER SCREEN_ELSE(GRect(12, 58, 18, 18), \
                                    PBL_IF_RECT_ELSE(GRect(9, 42, 13, 13), GRect(84, 52, 13, 13)), \
                                    GRect(84, 52, 13, 13))
#define BLUETOOTH_ICON_CIRCLE SCREEN_ELSE(GRect(0, 0, 18, 18), GRect(0, 0, 13, 13), GRect(0, 0, 13, 13))
#define BLUETOOTH_ICON_SYMBOL SCREEN_ELSE(GRect(3, 2, 10, 13), GRect(2, 1, 7, 9), GRect(2, 1, 7, 9))

//TIMEDIGITS
#define TIMEDIGITS_CENTER SCREEN_ELSE(GRect(0, 21, 200, 207), \
                                      PBL_IF_RECT_ELSE(GRect(0, 15, 144, 168), GRect(18, 5, 180, 180)), \
                                      GRect(18, 5, 180, 180))
#define TIMEDIGITS_DATE SCREEN_ELSE(GRect(0, 53, 192, 42), \
                                    PBL_IF_RECT_ELSE(GRect(0, 38, 138, 30), GRect(0, 7, 99, 30)), \
                                    GRect(0, 7, 99, 30))
#define TIMEDIGITS_AMPM SCREEN_ELSE(GRect(22, 68, 79, 42), \
                                    PBL_IF_RECT_ELSE(GRect(16, 49, 57, 30), GRect(19, 46, 57, 30)), \
                                    GRect(19, 46, 57, 30))
#define TIMEDIGITS_DIGIT1 SCREEN_ELSE(4, 3, 3)
#define TIMEDIGITS_DIGIT2 SCREEN_ELSE(47, 34, 34)
#define TIMEDIGITS_DIGIT3 SCREEN_ELSE(103, 74, 74)
#define TIMEDIGITS_DIGIT4 SCREEN_ELSE(146, 105, 105)
#define TIMEDIGITS_SEPARATOR SCREEN_ELSE(61, 44, 44)
#define TIMEDIGITS_OFFSET_TOP SCREEN_ELSE(57, 41, 41)
#define TIMEDIGITS_WIDTH SCREEN_ELSE(50, 36, 36)
#define TIMEDIGITS_HEIGHT SCREEN_ELSE(111, 80, 80)

//TIMDIGITS SECONDS
#define TIMEDIGITS_SECONDS_DIGIT1 SCREEN_ELSE(4, 3, 3)
#define TIMEDIGITS_SECONDS_DIGIT2 SCREEN_ELSE(39, 28, 28)
#define TIMEDIGITS_SECONDS_DIGIT3 SCREEN_ELSE(86, 62, 62)
#define TIMEDIGITS_SECONDS_DIGIT4 SCREEN_ELSE(121, 87, 87)
#define TIMEDIGITS_SECONDS_DIGIT5 SCREEN_ELSE(163, 117, 117)
#define TIMEDIGITS_SECONDS_DIGIT6 SCREEN_ELSE(178, 128, 128)
#define TIMEDIGITS_SECONDS_SEPARATOR SCREEN_ELSE(50, 36, 36)
#define TIMEDIGITS_SECONDS_OFFSET_TOP SCREEN_ELSE(72, 52, 52)
#define TIMEDIGITS_SECONDS_SMALL_OFFSET_TOP SCREEN_ELSE(106, 76, 76)
#define TIMEDIGITS_SECONDS_WIDTH SCREEN_ELSE(42, 30, 30)
#define TIMEDIGITS_SECONDS_SMALL_WIDTH SCREEN_ELSE(15, 11, 11)
#define TIMEDIGITS_SECONDS_HEIGHT SCREEN_ELSE(78, 56, 56)
#define TIMEDIGITS_SECONDS_SMALL_HEIGHT SCREEN_ELSE(31, 22, 22)

//DECORATIONS
#define DECORATIONS_LINE_TOP_START SCREEN_ELSE(GPoint(0,26), PBL_IF_RECT_ELSE(GPoint(0,19), GPoint(33,37)), GPoint(33,37))
#define DECORATIONS_LINE_TOP_END SCREEN_ELSE(GPoint(200,26), PBL_IF_RECT_ELSE(GPoint(144,19), GPoint(147,37)), GPoint(147,37))

#define DECORATIONS_LINE_BOTTOM_START SCREEN_ELSE(GPoint(0,208), PBL_IF_RECT_ELSE(GPoint(0,150), GPoint(32,142)), GPoint(32,142))
#define DECORATIONS_LINE_BOTTOM_END SCREEN_ELSE(GPoint(200,208), PBL_IF_RECT_ELSE(GPoint(144,150), GPoint(148,142)), GPoint(148,142))

#define DECORATIONS_BUTTON_BACK_LABEL SCREEN_ELSE(GRect(18, 29, 49, 14), GRect(13, 21, 35, 10), GRect(13, 21, 35, 10))
#define DECORATIONS_BUTTON_NEXT_LABEL SCREEN_ELSE(GRect(147, 190, 35, 14), GRect(106, 137, 25, 10), GRect(106, 137, 25, 10))
#define DECORATIONS_BUTTON_PREV_LABEL SCREEN_ELSE(GRect(147, 29, 35, 14), GRect(106, 21, 25, 10), GRect(106, 21, 25, 10))

#define DECORATIONS_BUTTON_BACK_ICON SCREEN_ELSE(GRect(7, 33, 7, 8), GRect(5, 24, 5, 6), GRect(5, 24, 5, 6))
#define DECORATIONS_BUTTON_NEXT_ICON SCREEN_ELSE(GRect(185, 33, 7, 8), GRect(133, 24, 5, 6), GRect(133, 24, 5, 6))
#define DECORATIONS_BUTTON_PREV_ICON SCREEN_ELSE(GRect(185, 195, 7, 8), GRect(133, 140, 5, 6), GRect(133, 140, 5, 6))

#define DECORATIONS_BRANDING SCREEN_ELSE(GRect(0, -6, 200, 42), GRect(0, -4, 144, 30), GRect(0, -4, 144, 30))

// Logo bitmap is used at its native size on every platform; the cropped
// "pebble"-only bitmap (52x15) is centered on each screen width.
#define DECORATIONS_LOGO SCREEN_ELSE(GRect(74, 3, 52, 15), PBL_IF_RECT_ELSE(GRect(46, 2, 52, 15), GRect(64, 2, 52, 15)), GRect(64, 2, 52, 15))

//WATER RESIST box (CM label removed; box currently decorative)
//WATER/RESIST labels are now day high/low temp readouts
#define DECORATIONS_WR_OUTER SCREEN_ELSE(GRect(71,206,58,22), PBL_IF_RECT_ELSE(GRect(51,148,42,16), GRect(69,138,42,16)), GRect(69,138,42,16))
#define DECORATIONS_TEMP_HI SCREEN_ELSE(GRect(0,211,64,28), PBL_IF_RECT_ELSE(GRect(0,152,46,20), GRect(19,128,46,20)), GRect(19,128,46,20))
#define DECORATIONS_TEMP_LO SCREEN_ELSE(GRect(138,211,67,28), PBL_IF_RECT_ELSE(GRect(99,152,48,20), GRect(116,128,48,20)), GRect(116,128,48,20))

// Vector path scale factor now lives in vector.h (with the pristine arrays).
