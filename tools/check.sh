#!/bin/sh
# Verify the firmware without PlatformIO: syntax-check every source against
# stub headers, then run the classifier tests with plain g++.
#
#   sh tools/check.sh
#
# This is NOT a substitute for `pio run` — it never touches a real toolchain,
# linker or the ESP32 headers. It catches syntax errors, bad signatures and
# broken classifier maths, which is what it was built for.
set -e
cd "$(dirname "$0")/.."
CXX=${CXX:-g++}
STUBS=tools/stubs
fail=0

echo "== syntax check (Arduino core 2.x and 3.x) =="
for v in 2 3; do
    for f in src/*.cpp; do
        if ! $CXX -fsyntax-only -std=gnu++17 -Wall -Wextra \
             -DESP_ARDUINO_VERSION_MAJOR=$v -Iinclude -I$STUBS "$f"; then
            echo "  FAILED: $f (core $v)"; fail=1
        fi
    done
done
[ $fail -eq 0 ] && echo "  clean, both cores"

echo "== classifier tests =="
$CXX -std=gnu++17 -Wall -Wextra -Iinclude -I$STUBS \
     test/test_classifier/test_classifier.cpp src/classifier.cpp -o /tmp/bt_tests
/tmp/bt_tests || fail=1

exit $fail
