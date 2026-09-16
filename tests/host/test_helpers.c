/* Unit tests for src/c/helpers.c pure functions. Host build. */
#include "pebble.h"
#include "test_util.h"

#include "../../src/c/settings.h"

/* declarations under test (from helpers.h) */
void duration_to_time(int duration_s, int *hours, int *minutes);
void format_commas(int n, char *out);
void format_date(DateFormat fmt, struct tm *t, char *out, size_t out_len);
uint8_t hex_to_num(char h);

/* 2026-09-25 was a Wednesday (tm_wday 3); 2027-01-03 a Sunday. */
static void set_date(struct tm *t, int year, int mon0, int day, int wday) {
  *t = (struct tm){ .tm_year = year - 1900, .tm_mon = mon0, .tm_mday = day, .tm_wday = wday };
}

static void test_format_date(void) {
  char out[24];
  struct tm t;
  set_date(&t, 2026, 8, 25, 3);   // Sep 25 2026, WED

  set_date(&t, 2026, 8, 25, 3);
  format_date(DATE_FMT_DDMMYY, &t, out, sizeof(out));   ASSERT_STR(out, "25/09/26", "dd/mm/yy");
  format_date(DATE_FMT_MMDDYY, &t, out, sizeof(out));   ASSERT_STR(out, "09/25/26", "mm/dd/yy");
  format_date(DATE_FMT_YYMMDD, &t, out, sizeof(out));   ASSERT_STR(out, "26-09-25", "yy-mm-dd");
  format_date(DATE_FMT_WEEKDAY_DD, &t, out, sizeof(out));   ASSERT_STR(out, "WED-25", "DAY-dd");
  format_date(DATE_FMT_MONTH_WEEKDAY_DD, &t, out, sizeof(out));   ASSERT_STR(out, "SEP-WED-25", "MONTH-DAY-dd");

  // single-digit day and month stay zero padded
  set_date(&t, 2027, 0, 3, 0);    // Jan 3 2027, SUN
  format_date(DATE_FMT_DDMMYY, &t, out, sizeof(out));   ASSERT_STR(out, "03/01/27", "single digits padded");
  format_date(DATE_FMT_MONTH_WEEKDAY_DD, &t, out, sizeof(out));   ASSERT_STR(out, "JAN-SUN-03", "names for JAN/SUN");

  // an unknown format value renders the default, never garbage
  set_date(&t, 2026, 8, 25, 3);
  format_date((DateFormat)200, &t, out, sizeof(out));
  ASSERT_STR(out, "09/25/26", "unknown format -> default");
}

static void test_duration_to_time(void) {
  int h, m;
  duration_to_time(0, &h, &m);        ASSERT_EQ(h, 0, "0s hours");  ASSERT_EQ(m, 0, "0s mins");
  duration_to_time(59, &h, &m);       ASSERT_EQ(h, 0, "59s hours"); ASSERT_EQ(m, 0, "59s mins");
  duration_to_time(60, &h, &m);       ASSERT_EQ(h, 0, "60s hours"); ASSERT_EQ(m, 1, "60s mins");
  duration_to_time(3600, &h, &m);     ASSERT_EQ(h, 1, "3600s hours"); ASSERT_EQ(m, 0, "3600s mins");
  duration_to_time(3661, &h, &m);     ASSERT_EQ(h, 1, "3661s hours"); ASSERT_EQ(m, 1, "3661s mins");
  duration_to_time(86399, &h, &m);    ASSERT_EQ(h, 23, "86399s hours"); ASSERT_EQ(m, 59, "86399s mins");
}

static void test_format_commas(void) {
  char out[32];
  format_commas(0, out);      ASSERT_STR(out, "0", "0");
  format_commas(5, out);      ASSERT_STR(out, "5", "5");
  format_commas(999, out);    ASSERT_STR(out, "999", "999");
  format_commas(1000, out);   ASSERT_STR(out, "1,000", "1000");
  format_commas(12345, out);  ASSERT_STR(out, "12,345", "12345");
  format_commas(1234567, out); ASSERT_STR(out, "1,234,567", "1234567");
  format_commas(500, out);    ASSERT_STR(out, "500", "500");
  format_commas(10000, out);  ASSERT_STR(out, "10,000", "10000");
}

static void test_hex_to_num(void) {
  ASSERT_EQ(hex_to_num('0'), 0, "0");
  ASSERT_EQ(hex_to_num('9'), 9, "9");
  ASSERT_EQ(hex_to_num('A'), 10, "A");
  ASSERT_EQ(hex_to_num('F'), 15, "F");
  ASSERT_EQ(hex_to_num('a'), 10, "a");
  ASSERT_EQ(hex_to_num('f'), 15, "f");
  ASSERT_EQ(hex_to_num('g'), 0, "g (invalid -> 0)");
  ASSERT_EQ(hex_to_num('Z'), 0, "Z (invalid -> 0)");
  ASSERT_EQ(hex_to_num(' '), 0, "space (invalid -> 0)");
}

int main(void) {
  RUN(test_duration_to_time);
  RUN(test_format_commas);
  RUN(test_format_date);
  RUN(test_hex_to_num);
  TEST_SUMMARY();
}
