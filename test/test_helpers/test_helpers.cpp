/*
 * Unit tests for helper functions
 *
 * Tests mapfloat() which is used throughout the codebase for
 * linear interpolation/mapping of values between ranges.
 */

#include <unity.h>
#include <cmath>

// Define mapfloat locally for testing (same implementation as Helpers.cpp)
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
    if ((in_max - in_min) + out_min == 0)
        return 0;
    else
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setUp(void) {}
void tearDown(void) {}

// =============================================================================
// Test: Basic mapping functionality
// =============================================================================

void test_mapfloat_identity(void) {
    // Map 0-100 to 0-100 should return same value
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, mapfloat(50.0f, 0.0f, 100.0f, 0.0f, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mapfloat(0.0f, 0.0f, 100.0f, 0.0f, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, mapfloat(100.0f, 0.0f, 100.0f, 0.0f, 100.0f));
}

void test_mapfloat_scale_up(void) {
    // Map 0-1 to 0-100
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mapfloat(0.0f, 0.0f, 1.0f, 0.0f, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, mapfloat(0.5f, 0.0f, 1.0f, 0.0f, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, mapfloat(1.0f, 0.0f, 1.0f, 0.0f, 100.0f));
}

void test_mapfloat_scale_down(void) {
    // Map 0-100 to 0-1
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mapfloat(0.0f, 0.0f, 100.0f, 0.0f, 1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, mapfloat(50.0f, 0.0f, 100.0f, 0.0f, 1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, mapfloat(100.0f, 0.0f, 100.0f, 0.0f, 1.0f));
}

void test_mapfloat_offset_ranges(void) {
    // Map 100-200 to 0-50
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mapfloat(100.0f, 100.0f, 200.0f, 0.0f, 50.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, mapfloat(150.0f, 100.0f, 200.0f, 0.0f, 50.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, mapfloat(200.0f, 100.0f, 200.0f, 0.0f, 50.0f));
}

void test_mapfloat_inverted_output(void) {
    // Map 0-100 to 100-0 (inverted)
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, mapfloat(0.0f, 0.0f, 100.0f, 100.0f, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, mapfloat(50.0f, 0.0f, 100.0f, 100.0f, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mapfloat(100.0f, 0.0f, 100.0f, 100.0f, 0.0f));
}

void test_mapfloat_negative_values(void) {
    // Map -10 to +10 to 0-100
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mapfloat(-10.0f, -10.0f, 10.0f, 0.0f, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, mapfloat(0.0f, -10.0f, 10.0f, 0.0f, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, mapfloat(10.0f, -10.0f, 10.0f, 0.0f, 100.0f));
}

void test_mapfloat_extrapolation_above(void) {
    // Input above range should extrapolate
    float result = mapfloat(150.0f, 0.0f, 100.0f, 0.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, result);
}

void test_mapfloat_extrapolation_below(void) {
    // Input below range should extrapolate
    float result = mapfloat(-50.0f, 0.0f, 100.0f, 0.0f, 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -5.0f, result);
}

// =============================================================================
// Test: Edge cases
// =============================================================================

void test_mapfloat_zero_input_range(void) {
    // When in_max == in_min, the function has special handling
    // The condition checks: (in_max - in_min) + out_min == 0
    // This is a quirky condition - let's test what happens

    // Case: in_min=in_max=0, out_min=0 -> returns 0 (divide by zero protection)
    float result = mapfloat(5.0f, 0.0f, 0.0f, 0.0f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

void test_mapfloat_realistic_audio_pps(void) {
    // Realistic use case from Audio.cpp: mapping AOA to pulse-per-second
    // HIGH_TONE_PPS range is 1.5 to 6.2
    float onSpeedSlowAOA = 8.5f;
    float stallWarnAOA = 15.0f;

    // At onSpeedSlowAOA, should be at minimum PPS
    float pps_min = mapfloat(onSpeedSlowAOA, onSpeedSlowAOA, stallWarnAOA, 1.5f, 6.2f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.5f, pps_min);

    // At stallWarnAOA, should be at maximum PPS
    float pps_max = mapfloat(stallWarnAOA, onSpeedSlowAOA, stallWarnAOA, 1.5f, 6.2f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.2f, pps_max);

    // At midpoint AOA, should be at midpoint PPS
    float midAOA = (onSpeedSlowAOA + stallWarnAOA) / 2.0f;
    float pps_mid = mapfloat(midAOA, onSpeedSlowAOA, stallWarnAOA, 1.5f, 6.2f);
    float expected_mid = (1.5f + 6.2f) / 2.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expected_mid, pps_mid);
}

void test_mapfloat_realistic_volume(void) {
    // Realistic use case: mapping analog pot value to volume percent
    // Analog range might be 0-4095, volume 0-100%
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, mapfloat(0.0f, 0.0f, 4095.0f, 0.0f, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, mapfloat(2047.5f, 0.0f, 4095.0f, 0.0f, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, mapfloat(4095.0f, 0.0f, 4095.0f, 0.0f, 100.0f));
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(test_mapfloat_identity);
    RUN_TEST(test_mapfloat_scale_up);
    RUN_TEST(test_mapfloat_scale_down);
    RUN_TEST(test_mapfloat_offset_ranges);
    RUN_TEST(test_mapfloat_inverted_output);
    RUN_TEST(test_mapfloat_negative_values);
    RUN_TEST(test_mapfloat_extrapolation_above);
    RUN_TEST(test_mapfloat_extrapolation_below);

    // Edge cases
    RUN_TEST(test_mapfloat_zero_input_range);

    // Realistic use cases
    RUN_TEST(test_mapfloat_realistic_audio_pps);
    RUN_TEST(test_mapfloat_realistic_volume);

    return UNITY_END();
}
