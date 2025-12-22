/*
 * Unit tests for Pressure Calculations
 *
 * Tests the conversion of raw ADC counts from Honeywell HSC pressure sensors
 * to engineering units (PSI, millibars).
 *
 * Sensor specs (from HscPressureSensor.cpp):
 *   - 12-bit ADC (0-4095 counts)
 *   - Output range: 10% to 90% (409 to 3686 counts)
 *   - HSCDRNN1.6BASA3 (diff): -1.0 to +1.0 PSI
 *   - HSCDRRN100MDSA3 (abs): 0.0 to 23.2 PSI
 */

#include <unity.h>
#include <cmath>

#include <PressureCalc.h>

void setUp(void) {}
void tearDown(void) {}

// =============================================================================
// Test: Differential pressure sensor (AOA/Pitot)
// =============================================================================

void test_diff_counts_at_minimum(void) {
    // At 409 counts (10%), pressure should be -1.0 PSI
    float psi = diffCountsToPSI(COUNTS_MIN_DIFF);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, PRESSURE_MIN_DIFF, psi);
}

void test_diff_counts_at_maximum(void) {
    // At 3686 counts (90%), pressure should be +1.0 PSI
    float psi = diffCountsToPSI(COUNTS_MAX_DIFF);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, PRESSURE_MAX_DIFF, psi);
}

void test_diff_counts_at_midpoint(void) {
    // At midpoint counts, pressure should be 0 PSI (no differential)
    unsigned midCounts = (COUNTS_MIN_DIFF + COUNTS_MAX_DIFF) / 2;
    float psi = diffCountsToPSI(midCounts);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, psi);
}

void test_diff_counts_quarter_scale(void) {
    // At 25% of range, pressure should be -0.5 PSI
    unsigned counts = COUNTS_MIN_DIFF + (COUNTS_MAX_DIFF - COUNTS_MIN_DIFF) / 4;
    float psi = diffCountsToPSI(counts);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -0.5f, psi);
}

void test_diff_counts_three_quarter_scale(void) {
    // At 75% of range, pressure should be +0.5 PSI
    unsigned counts = COUNTS_MIN_DIFF + 3 * (COUNTS_MAX_DIFF - COUNTS_MIN_DIFF) / 4;
    float psi = diffCountsToPSI(counts);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, psi);
}

// =============================================================================
// Test: Absolute pressure sensor (Static)
// =============================================================================

void test_abs_counts_at_minimum(void) {
    // At 409 counts (10%), pressure should be 0 PSI (vacuum)
    float psi = absCountsToPSI(COUNTS_MIN_ABS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, PRESSURE_MIN_ABS, psi);
}

void test_abs_counts_at_maximum(void) {
    // At 3686 counts (90%), pressure should be 23.2 PSI
    float psi = absCountsToPSI(COUNTS_MAX_ABS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, PRESSURE_MAX_ABS, psi);
}

void test_abs_counts_at_midpoint(void) {
    // At midpoint counts, pressure should be ~11.6 PSI
    unsigned midCounts = (COUNTS_MIN_ABS + COUNTS_MAX_ABS) / 2;
    float psi = absCountsToPSI(midCounts);
    float expectedPsi = (PRESSURE_MIN_ABS + PRESSURE_MAX_ABS) / 2.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expectedPsi, psi);
}

void test_abs_counts_to_millibars(void) {
    // Test millibar conversion at a known point
    // Standard sea level: 14.7 PSI = 1013.25 mbar
    // Find counts for ~14.7 PSI
    // 14.7 PSI is at: (14.7 - 0) / (23.2 - 0) * (3686 - 409) + 409 = 2484 counts
    unsigned counts = 2484;
    float mbar = absCountsToMillibars(counts);
    // Expected: 14.7 * 68.9476 ≈ 1013 mbar
    TEST_ASSERT_FLOAT_WITHIN(20.0f, 1013.0f, mbar);
}

// =============================================================================
// Test: Unit conversions
// =============================================================================

void test_psi_to_millibars(void) {
    // 1 PSI = 68.9476 mbar
    float mbar = psiToMillibars(1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 68.9476f, mbar);
}

void test_psi_to_millibars_standard_atmosphere(void) {
    // Standard atmosphere: 14.696 PSI = 1013.25 mbar
    float mbar = psiToMillibars(14.696f);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 1013.25f, mbar);
}

void test_millibars_to_inhg(void) {
    // Standard atmosphere: 1013.25 mbar = 29.92 inHg
    float inHg = millibarsToInHg(1013.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 29.92f, inHg);
}

void test_zero_psi_to_millibars(void) {
    float mbar = psiToMillibars(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mbar);
}

// =============================================================================
// Test: Edge cases and out-of-range values
// =============================================================================

void test_diff_counts_below_minimum(void) {
    // Below 10% - should extrapolate (sensor saturated low)
    // At 200 counts: (200-409)/(3686-409) * 2.0 + (-1.0) = -1.127
    float psi = diffCountsToPSI(200);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -1.127f, psi);
}

void test_diff_counts_above_maximum(void) {
    // Above 90% - should extrapolate (sensor saturated high)
    // At 4000 counts: (4000-409)/(3686-409) * 2.0 + (-1.0) = 1.19
    float psi = diffCountsToPSI(4000);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.19f, psi);
}

void test_diff_counts_at_zero(void) {
    // At 0 counts - extrapolates way below range
    // (0-409)/(3686-409) * 2.0 + (-1.0) = -1.25
    float psi = diffCountsToPSI(0);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -1.25f, psi);
}

void test_abs_counts_at_zero(void) {
    // At 0 counts - extrapolates below vacuum (physically impossible but mathematically valid)
    float psi = absCountsToPSI(0);
    TEST_ASSERT_LESS_THAN(0.0f, psi);
}

// =============================================================================
// Test: Realistic flight scenarios
// =============================================================================

void test_sea_level_static_pressure(void) {
    // At sea level, expect ~14.7 PSI
    // This would be counts: (14.7/23.2) * (3686-409) + 409 ≈ 2484
    float psi = absCountsToPSI(2484);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 14.7f, psi);
}

void test_10000ft_static_pressure(void) {
    // At 10,000 ft, static pressure is ~10.1 PSI
    // Counts: (10.1/23.2) * (3686-409) + 409 ≈ 1835
    float psi = absCountsToPSI(1835);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.1f, psi);
}

void test_typical_aoa_differential(void) {
    // Typical AOA sensor differential at moderate AOA might be ~0.1 PSI
    // Counts: ((0.1 - (-1.0)) / 2.0) * (3686-409) + 409 ≈ 2211
    float psi = diffCountsToPSI(2211);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.1f, psi);
}

void test_pitot_differential_100kts(void) {
    // At ~100 kts, pitot-static differential is roughly 0.5 PSI
    // This is a rough estimate for testing purposes
    unsigned counts = COUNTS_MIN_DIFF + 3 * (COUNTS_MAX_DIFF - COUNTS_MIN_DIFF) / 4;
    float psi = diffCountsToPSI(counts);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.5f, psi);
}

// =============================================================================
// Main test runner
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Differential pressure sensor
    RUN_TEST(test_diff_counts_at_minimum);
    RUN_TEST(test_diff_counts_at_maximum);
    RUN_TEST(test_diff_counts_at_midpoint);
    RUN_TEST(test_diff_counts_quarter_scale);
    RUN_TEST(test_diff_counts_three_quarter_scale);

    // Absolute pressure sensor
    RUN_TEST(test_abs_counts_at_minimum);
    RUN_TEST(test_abs_counts_at_maximum);
    RUN_TEST(test_abs_counts_at_midpoint);
    RUN_TEST(test_abs_counts_to_millibars);

    // Unit conversions
    RUN_TEST(test_psi_to_millibars);
    RUN_TEST(test_psi_to_millibars_standard_atmosphere);
    RUN_TEST(test_millibars_to_inhg);
    RUN_TEST(test_zero_psi_to_millibars);

    // Edge cases
    RUN_TEST(test_diff_counts_below_minimum);
    RUN_TEST(test_diff_counts_above_maximum);
    RUN_TEST(test_diff_counts_at_zero);
    RUN_TEST(test_abs_counts_at_zero);

    // Realistic scenarios
    RUN_TEST(test_sea_level_static_pressure);
    RUN_TEST(test_10000ft_static_pressure);
    RUN_TEST(test_typical_aoa_differential);
    RUN_TEST(test_pitot_differential_100kts);

    return UNITY_END();
}
