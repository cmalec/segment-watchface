/* Unit tests for src/c/helpers.c pure functions. Host build. */
#include "pebble.h"
#include "test_util.h"

/* declarations under test (from helpers.h) */
void duration_to_time(int duration_s, int *hours, int *minutes);
void format_commas(int n, char *out);
uint8_t hex_to_num(char h);

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
  RUN(test_hex_to_num);
  TEST_SUMMARY();
}
