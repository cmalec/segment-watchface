#!/bin/sh
# Host-side unit tests for the Segment watchface. Compiles the pure logic in
# src/c/ against tests/host/pebble.h (a minimal mock) and runs each binary.
# No SDK / ARM toolchain needed. Usage: tests/run.sh  (from repo root)
set -e
cd "$(dirname "$0")/.."

CC=${CC:-cc}
CFLAGS="-std=c11 -Wall -Wextra -Wno-unused-parameter -Itests/host -Isrc/c"
OUT=tests/host/build
mkdir -p "$OUT"

fail=0

build_and_run() {
  name=$1; plat=$2; shift 2
  srcs="$@"
  echo "== $name ($plat) =="
  $CC $CFLAGS -D$plat -o "$OUT/$name" $srcs tests/host/mock_persist.c
  "./$OUT/$name" || fail=1
}

# Segment targets Emery only. Build each pure suite against the Emery mock so
# the layout contract cannot silently regress to a legacy form factor.
build_and_run test_helpers_emery PBL_PLATFORM_EMERY tests/host/test_helpers.c src/c/helpers.c

# settings range logic
build_and_run test_settings_emery PBL_PLATFORM_EMERY tests/host/test_settings.c src/c/settings.c src/c/helpers.c

# color packing contract
build_and_run test_colors_emery PBL_PLATFORM_EMERY tests/host/test_colors.c src/c/helpers.c

# native Emery layout invariants
build_and_run test_layout_emery PBL_PLATFORM_EMERY tests/host/test_layout.c

echo
if [ $fail -eq 0 ]; then echo "ALL TESTS PASSED"; else echo "SOME TESTS FAILED"; exit 1; fi
