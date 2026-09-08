/* Tiny unit-test assertion + runner helpers for host tests. */
#pragma once
#include <stdio.h>
#include <string.h>

static int g_pass = 0, g_fail = 0;
static const char *g_current = "";

#define RUN(fn) do { g_current = #fn; fn(); } while (0)

#define ASSERT_TRUE(cond, msg) do { \
  if (cond) { g_pass++; } \
  else { g_fail++; printf("FAIL [%s] %s: %s (line %d)\n", g_current, #cond, msg, __LINE__); } \
} while (0)

#define ASSERT_EQ(a, b, msg) do { \
  long _a = (long)(a), _b = (long)(b); \
  if (_a == _b) { g_pass++; } \
  else { g_fail++; printf("FAIL [%s] %s: %s (got %ld, want %ld, line %d)\n", g_current, #a, msg, _a, _b, __LINE__); } \
} while (0)

#define ASSERT_STR(a, b, msg) do { \
  if (strcmp((a), (b)) == 0) { g_pass++; } \
  else { g_fail++; printf("FAIL [%s]: %s (got \"%s\", want \"%s\", line %d)\n", g_current, msg, (a), (b), __LINE__); } \
} while (0)

#define TEST_SUMMARY() do { \
  printf("\n%d passed, %d failed\n", g_pass, g_fail); \
  return g_fail ? 1 : 0; \
} while (0)
