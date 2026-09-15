// Minimal Unity shim so test/test_classifier runs under plain g++ when
// PlatformIO is unavailable. Implements only the assertions that test uses.
// `pio test -e native` still uses the real Unity; this exists so the same test
// file has a second runner rather than a duplicated copy of the logic.
#pragma once
#include <cmath>
#include <cstdio>

static int unity_failures = 0;
static const char* unity_current = "";

#define UNITY_BEGIN() (unity_failures = 0)
#define UNITY_END()   (printf(unity_failures ? "\n%d TEST(S) FAILED\n" \
                                             : "\nall tests passed\n", unity_failures), \
                       unity_failures)
#define RUN_TEST(fn)  do { unity_current = #fn; fn(); } while (0)

#define UNITY_FAIL(fmt, ...) \
    do { printf("FAIL %s (%s:%d): " fmt "\n", unity_current, __FILE__, __LINE__, \
                ##__VA_ARGS__); ++unity_failures; } while (0)

#define TEST_ASSERT_TRUE(c) \
    do { if (!(c)) UNITY_FAIL("expected true: %s", #c); } while (0)
#define TEST_ASSERT_EQUAL(e, a) \
    do { if ((long)(e) != (long)(a)) \
            UNITY_FAIL("%s expected %ld got %ld", #a, (long)(e), (long)(a)); } while (0)
#define TEST_ASSERT_FLOAT_WITHIN(d, e, a) \
    do { if (std::fabs((double)(e) - (double)(a)) > (double)(d)) \
            UNITY_FAIL("%s expected %g +/- %g got %g", #a, (double)(e), (double)(d), \
                       (double)(a)); } while (0)
#define TEST_ASSERT_EQUAL_FLOAT(e, a) TEST_ASSERT_FLOAT_WITHIN(1e-5, e, a)
