// Host-side tests for the classifier maths.  pio test -e native
#include <unity.h>
#include <cmath>
#include "classifier.h"

using namespace bt;

static PanelCal cal() {
    return PanelCal{/*ratio_cutoff*/ 0.55f, /*sigma_intensity*/ 0.012f,
                    /*control_min*/ 0.18f, /*confidence_min*/ 0.95f};
}

// A strong test line on a competitive strip means NO drug.
void test_strong_test_line_reads_clear() {
    PanelResult r = classify(LineRead{0.80f, 0.90f}, cal());
    TEST_ASSERT_EQUAL(static_cast<int>(Verdict::Clear), static_cast<int>(r.verdict));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.8889f, r.ratio);
    TEST_ASSERT_TRUE(r.confidence > 0.999f);
}

// A faint test line means the drug blocked the conjugate: present.
void test_faint_test_line_reads_detected() {
    PanelResult r = classify(LineRead{0.20f, 0.85f}, cal());
    TEST_ASSERT_EQUAL(static_cast<int>(Verdict::Detected), static_cast<int>(r.verdict));
    TEST_ASSERT_TRUE(r.ratio < 0.55f);
    TEST_ASSERT_TRUE(r.confidence > 0.999f);
}

// Sitting on the cutoff must not produce a verdict in either direction.
void test_ratio_on_the_cutoff_is_inconclusive() {
    PanelResult r = classify(LineRead{0.468f, 0.85f}, cal());
    TEST_ASSERT_EQUAL(static_cast<int>(Verdict::Inconclusive), static_cast<int>(r.verdict));
    TEST_ASSERT_TRUE(r.confidence < 0.95f);
}

// No control line is a failed strip, which is NOT the same as inconclusive:
// the operator fits a new strip instead of escalating to a lab.
void test_missing_control_line_is_invalid() {
    PanelResult r = classify(LineRead{0.80f, 0.10f}, cal());
    TEST_ASSERT_EQUAL(static_cast<int>(Verdict::Invalid), static_cast<int>(r.verdict));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, r.confidence);
}

// The 95% confidence floor must land exactly on the textbook 1.96 sigma, which
// is what makes the inconclusive band defensible rather than arbitrary.
void test_confidence_floor_is_1_96_sigma() {
    const float cutoff = 0.55f, sigma = 0.02f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.95f,
        read_confidence(cutoff + 1.96f * sigma, sigma, cutoff));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.95f,
        read_confidence(cutoff - 1.96f * sigma, sigma, cutoff));
}

// Noise must be propagated from both lines, not assumed on the ratio. A dim
// test line carries more relative noise, so the same ratio is less certain.
void test_ratio_sigma_propagates_from_both_lines() {
    const float bright = ratio_sigma(LineRead{0.80f, 0.90f}, 0.012f);
    const float dim    = ratio_sigma(LineRead{0.08f, 0.09f}, 0.012f);
    TEST_ASSERT_TRUE(dim > bright * 5.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0005f, 0.01784f, bright);
}

// Degenerate reads must fail closed, never divide by zero into a verdict.
void test_zero_lines_fail_closed() {
    TEST_ASSERT_EQUAL(static_cast<int>(Verdict::Invalid),
        static_cast<int>(classify(LineRead{0.0f, 0.0f}, cal()).verdict));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, read_confidence(0.6f, 0.0f, 0.55f));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_strong_test_line_reads_clear);
    RUN_TEST(test_faint_test_line_reads_detected);
    RUN_TEST(test_ratio_on_the_cutoff_is_inconclusive);
    RUN_TEST(test_missing_control_line_is_invalid);
    RUN_TEST(test_confidence_floor_is_1_96_sigma);
    RUN_TEST(test_ratio_sigma_propagates_from_both_lines);
    RUN_TEST(test_zero_lines_fail_closed);
    return UNITY_END();
}
