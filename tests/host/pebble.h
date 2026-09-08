/*
 * Minimal host-side mock of the Pebble SDK, sufficient to compile and unit
 * test the pure logic in src/c/ (settings ranges, comma/hex formatting,
 * color packing). NOT a full SDK — only the pieces the tested code touches.
 *
 * Define exactly one of PBL_PLATFORM_APLITE / PBL_PLATFORM_EMERY per test
 * binary to exercise platform-gated paths.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>

/* ---- geometry ---- */
typedef struct { int16_t x, y; } GPoint;
typedef struct { int16_t w, h; } GSize;
typedef struct GRect { GPoint origin; GSize size; } GRect;
#define GRect(x, y, w, h) ((GRect){ .origin = GPoint(x, y), .size = GSize(w, h) })

/* ---- color ---- */
typedef struct GColor8 { uint8_t argb; } GColor8;
typedef GColor8 GColor;
#define GColorBlack   ((GColor){ .argb = 0b11000000 })
#define GColorWhite   ((GColor){ .argb = 0b11111111 })
#define GColorClear   ((GColor){ .argb = 0b00000000 })
#define GColorOrange  ((GColor){ .argb = 0b11110100 })
#define GColorRed     ((GColor){ .argb = 0b11000011 })
#define GColorDukeBlue ((GColor){ .argb = 0b11001010 })
#define GColorLightGray ((GColor){ .argb = 0b11101010 })
static inline bool gcolor_equal(GColor a, GColor b) { return a.argb == b.argb; }

/* ---- platform feature macros (mirrors SDK) ---- */
#if defined(PBL_PLATFORM_APLITE) || defined(PBL_PLATFORM_DIORITE)
  #define PBL_BW
  #define PBL_RECT
  #define PBL_DISPLAY_WIDTH  144
  #define PBL_DISPLAY_HEIGHT 168
#elif defined(PBL_PLATFORM_EMERY)
  #define PBL_COLOR
  #define PBL_RECT
  #define PBL_HEALTH
  #define PBL_DISPLAY_WIDTH  200
  #define PBL_DISPLAY_HEIGHT 228
#else /* basalt/chalk default for tests */
  #define PBL_COLOR
  #define PBL_RECT
  #define PBL_DISPLAY_WIDTH  144
  #define PBL_DISPLAY_HEIGHT 168
#endif

#define PBL_IF_COLOR_ELSE(c, bw)  (gcolor_placeholder_choose(c, bw))
static inline GColor gcolor_placeholder_choose(GColor c, GColor bw) {
  #ifdef PBL_COLOR
    return c;
  #else
    (void)c; return bw;
  #endif
}

/* ---- logging ---- */
#define APP_LOG(level, fmt, ...) do { fprintf(stderr, "[log] " fmt "\n", ##__VA_ARGS__); } while (0)
#define APP_LOG_LEVEL_INFO 0
#define APP_LOG_LEVEL_WARNING 1
#define APP_LOG_LEVEL_DEBUG 2

/* ---- persist (in-memory) ---- */
#define PERSIST_DATA_MAX_LENGTH 256
int persist_exists(uint32_t key);
int persist_read_data(uint32_t key, void *buffer, size_t buffer_size);
int persist_write_data(uint32_t key, const void *data, size_t size);
int persist_delete(uint32_t key);

/* ---- heap ---- */
int heap_bytes_free(void);

/* ---- app message (stub) ---- */
typedef struct DictionaryIterator DictionaryIterator;
typedef struct { uint32_t key; union { uint8_t uint8; int8_t int8; uint16_t uint16; int32_t int32; const char *cstring; } *value; } Tuple;
typedef enum { APP_MSG_OK = 0, APP_MSG_BUSY = 64 } AppMessageResult;
void app_message_register_inbox_received(void *cb);
void app_message_register_inbox_dropped(void *cb);
int app_message_open(uint32_t inbound, uint32_t outbound);
Tuple *dict_read_first(DictionaryIterator *iter);
Tuple *dict_read_next(DictionaryIterator *iter);

/* ---- accelerometer tap ---- */
typedef int AccelAxisType;
void accel_tap_service_subscribe(void *handler);
void accel_tap_service_unsubscribe(void);

/* ---- app timer ---- */
typedef struct AppTimer AppTimer;
typedef void (*AppTimerCallback)(void *data);
AppTimer *app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void *callback_data);
bool app_timer_cancel(AppTimer *timer);

/* ---- window/layer stubs (only used transitively by headers) ---- */
typedef struct Window Window;
typedef struct Layer Layer;
typedef struct TextLayer TextLayer;
typedef struct GContext GContext;
typedef void *GFont;
typedef enum { GTextAlignmentLeft, GTextAlignmentCenter, GTextAlignmentRight } GTextAlignment;
typedef struct { uint8_t charge_percent; bool is_charging; bool is_plugged; } BatteryChargeState;

/* MESSAGE_KEY_* resolve to the SDK's 10000+ numbering for the wire contract */
#define MESSAGE_KEY_blink 10000
#define MESSAGE_KEY_invert 10001
#define MESSAGE_KEY_bluetoothvibe 10002
#define MESSAGE_KEY_bluetoothshow 10003
#define MESSAGE_KEY_hourlyvibe 10004
#define MESSAGE_KEY_branding_mask 10005
#define MESSAGE_KEY_battery_hide 10006
#define MESSAGE_KEY_seconds 10007
#define MESSAGE_KEY_powersave 10008
#define MESSAGE_KEY_ps_start 10009
#define MESSAGE_KEY_ps_end 10010
#define MESSAGE_KEY_switchset 10011
#define MESSAGE_KEY_switch_start 10012
#define MESSAGE_KEY_switch_end 10013
#define MESSAGE_KEY_health 10014
#define MESSAGE_KEY_wtemp_req 10015
#define MESSAGE_KEY_wtemp_hi 10016
#define MESSAGE_KEY_wtemp_lo 10017
#define MESSAGE_KEY_pbatt_level 10018
#define MESSAGE_KEY_battery_icon_only 10022
#define MESSAGE_KEY_temp_unit 10023
#define MESSAGE_KEY_setcolors 10019
#define MESSAGE_KEY_set1colors 10020
#define MESSAGE_KEY_set2colors 10021
