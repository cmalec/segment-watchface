#include <pebble.h>
#include "helpers.h"
#include "settings.h"

void duration_to_time(int duration_s, int *hours, int *minutes) {
  *hours = duration_s / 3600;
  *minutes = (duration_s % 3600) / 60;
}

void format_commas(int n, char *out)
{
    int c;
    char buf[20];
    char *p;

    snprintf(buf, sizeof(buf), "%d", n);
    c = 2 - strlen(buf) % 3;
    for (p = buf; *p != 0; p++) {
       *out++ = *p;
       if (c == 1) {
           *out++ = ',';
       }
       c = (c + 1) % 3;
    }
    *--out = 0;
}

uint8_t hex_to_num (char h){
  uint8_t rslt=0;
  if ((h>='0') && (h<='9'))
    rslt= h-'0';
  else if ((h>='A') && (h<='F'))
    rslt = h-'A'+10;
  else if ((h>='a') && (h<='f'))
    rslt = h-'a'+10;
  return rslt;
}

// Emery-only target: PBL_COLOR is always defined, so the single XOR
// (`^ 0b00111111`, flipping the 6 color bits and leaving alpha untouched)
// covers Black<->White and every other color. GColorClear is exempt so an
// alpha-zero color stays transparent.
GColor color_helper(GColor color, uint8_t inverted) {
  if(inverted && !gcolor_equal(color, GColorClear)) {
    color.argb = color.argb ^ 0b00111111;
  }
  return color;
}

/*
 * Date text for the second row, in the format the user picked.
 *
 * Weekday and month names are spelled out here rather than taken from
 * strftime's locale: the font's character set is ASCII (any locale whose
 * abbreviations carry accents or non-Latin script would render blanks) and the
 * rest of the face's chrome is English anyway. The numeric parts use tm fields
 * directly so the format does not depend on the system locale either.
 *
 * Widths: the longest form ("SEP-WED-25") is 10 glyphs, 80px in Lucida 14,
 * inside the 150px date box.
 */
static const char *const WEEKDAY_NAMES[7] = {
  "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};
static const char *const MONTH_NAMES[12] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

void format_date(DateFormat fmt, struct tm *t, char *out, size_t out_len) {
  int day = t->tm_mday, month = t->tm_mon + 1, year = t->tm_year % 100;
  switch (fmt) {
    case DATE_FMT_DDMMYY:
      snprintf(out, out_len, "%02d/%02d/%02d", day, month, year);
      break;
    case DATE_FMT_YYMMDD:
      snprintf(out, out_len, "%02d-%02d-%02d", year, month, day);
      break;
    case DATE_FMT_WEEKDAY_DD:
      snprintf(out, out_len, "%s-%02d", WEEKDAY_NAMES[t->tm_wday], day);
      break;
    case DATE_FMT_MONTH_WEEKDAY_DD:
      snprintf(out, out_len, "%s-%s-%02d", MONTH_NAMES[t->tm_mon],
               WEEKDAY_NAMES[t->tm_wday], day);
      break;
    case DATE_FMT_MMDDYY:
    default:
      snprintf(out, out_len, "%02d/%02d/%02d", month, day, year);
      break;
  }
}

