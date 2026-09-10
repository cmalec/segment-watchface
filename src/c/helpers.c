#include <pebble.h>
#include "helpers.h"

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
