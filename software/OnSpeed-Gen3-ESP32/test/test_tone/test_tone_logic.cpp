/*
 * Unit tests for AOA Tone Logic
 *
 * These tests verify the critical audio tone selection logic that helps
 * pilots maintain safe flight. The tone logic maps angle of attack (AOA)
 * to audio tones:
 *
 *   - HIGH tone (1600 Hz): Slow / approaching stall
 *   - LOW tone (400 Hz): On speed / optimal approach
 *   - No tone: Too fast or airspeed too low
 *
 * The pulse rate increases as the pilot gets further from optimal:
 *   - Continuous tone = on speed (optimal)
 *   - Slow pulse = slightly off
 *   - Fast pulse = significantly off / stall warning
 */

#include <unity.h>
#include <cmath>

#include <ToneLogic.h>

// Typical flap configuration for a light aircraft (flaps up)
// These values are representative of a typical OnSpeed calibration
static const FlapAOAConfig FLAPS_UP = {
    .ldMaxAOA = 4.0f,         // L/D max (best glide)
    .onSpeedFastAOA = 7.0f,   // On speed range starts
    .onSpeedSlowAOA = 8.5f,   // On speed range ends
    .stallWarnAOA = 15.0f     // Stall warning
};

// Full flaps configuration (approach/landing)
static const FlapAOAConfig FLAPS_FULL = {
    .ldMaxAOA = 3.0f,
    .onSpeedFastAOA = 5.5f,
    .onSpeedSlowAOA = 7.0f,
    .stallWarnAOA = 12.0f
};

// Edge case: L/D max >= onSpeedFast (skips pulsed low tone region)
static const FlapAOAConfig FLAPS_EDGE = {
    .ldMaxAOA = 6.0f,         // Higher than onSpeedFast
    .onSpeedFastAOA = 5.5f,
    .onSpeedSlowAOA = 7.0f,
    .stallWarnAOA = 12.0f
};

static const float MUTE_IAS = 30.0f;  // Mute below 30 kts
static const float NORMAL_IAS = 80.0f;

void setUp(void) {}
void tearDown(void) {}

// =============================================================================
// Test: Audio muting at low airspeed
// =============================================================================

void test_tone_muted_below_mute_ias(void) {
    // Taxiing - no tone regardless of AOA
    ToneResult result = computeTone(10.0f, 25.0f, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::None, result.tone);
}

void test_tone_muted_at_exactly_mute_ias(void) {
    // At exactly the mute threshold - still muted
    ToneResult result = computeTone(10.0f, MUTE_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::None, result.tone);
}

void test_tone_active_above_mute_ias(void) {
    // Just above mute threshold - tones should work
    ToneResult result = computeTone(8.0f, MUTE_IAS + 1.0f, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);  // On speed
}

// =============================================================================
// Test: No tone region (too fast)
// =============================================================================

void test_no_tone_below_ldmax(void) {
    // AOA below L/D max = too fast, no tone
    ToneResult result = computeTone(2.0f, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::None, result.tone);
}

void test_no_tone_at_zero_aoa(void) {
    // Very low AOA (diving or high speed cruise)
    ToneResult result = computeTone(0.0f, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::None, result.tone);
}

void test_no_tone_negative_aoa(void) {
    // Negative AOA (steep dive)
    ToneResult result = computeTone(-5.0f, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::None, result.tone);
}

// =============================================================================
// Test: Pulsed LOW tone region (between L/D max and on-speed)
// =============================================================================

void test_pulsed_low_at_ldmax(void) {
    // At L/D max - low tone starts pulsing
    ToneResult result = computeTone(FLAPS_UP.ldMaxAOA, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, LOW_TONE_PPS_MIN, result.pulseFreq);
}

void test_pulsed_low_approaches_onspeed(void) {
    // Just before on-speed - pulse rate should be higher
    float aoa = FLAPS_UP.onSpeedFastAOA - 0.1f;
    ToneResult result = computeTone(aoa, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_GREATER_THAN(LOW_TONE_PPS_MIN, result.pulseFreq);
}

void test_pulsed_low_midpoint(void) {
    // Midpoint of pulsed low region
    float aoa = (FLAPS_UP.ldMaxAOA + FLAPS_UP.onSpeedFastAOA) / 2.0f;
    ToneResult result = computeTone(aoa, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    float expectedPPS = (LOW_TONE_PPS_MIN + LOW_TONE_PPS_MAX) / 2.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.5f, expectedPPS, result.pulseFreq);
}

// =============================================================================
// Test: Steady LOW tone region (on speed - optimal)
// =============================================================================

void test_steady_low_at_onspeed_fast(void) {
    // At on-speed fast boundary - steady low tone
    ToneResult result = computeTone(FLAPS_UP.onSpeedFastAOA, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.pulseFreq);  // Continuous
}

void test_steady_low_at_onspeed_slow(void) {
    // At on-speed slow boundary - still steady low tone
    ToneResult result = computeTone(FLAPS_UP.onSpeedSlowAOA, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.pulseFreq);
}

void test_steady_low_middle_of_onspeed(void) {
    // Middle of on-speed range
    float aoa = (FLAPS_UP.onSpeedFastAOA + FLAPS_UP.onSpeedSlowAOA) / 2.0f;
    ToneResult result = computeTone(aoa, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.pulseFreq);
}

// =============================================================================
// Test: Pulsed HIGH tone region (slow, approaching stall)
// =============================================================================

void test_pulsed_high_just_above_onspeed_slow(void) {
    // Just above on-speed slow - high tone starts
    float aoa = FLAPS_UP.onSpeedSlowAOA + 0.1f;
    ToneResult result = computeTone(aoa, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::High, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, HIGH_TONE_PPS_MIN, result.pulseFreq);
}

void test_pulsed_high_midpoint(void) {
    // Midpoint between on-speed slow and stall warning
    float aoa = (FLAPS_UP.onSpeedSlowAOA + FLAPS_UP.stallWarnAOA) / 2.0f;
    ToneResult result = computeTone(aoa, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::High, result.tone);
    float expectedPPS = (HIGH_TONE_PPS_MIN + HIGH_TONE_PPS_MAX) / 2.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.5f, expectedPPS, result.pulseFreq);
}

void test_pulsed_high_approaching_stall(void) {
    // Just below stall warning - pulse rate increasing
    float aoa = FLAPS_UP.stallWarnAOA - 0.5f;
    ToneResult result = computeTone(aoa, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::High, result.tone);
    TEST_ASSERT_GREATER_THAN(HIGH_TONE_PPS_MIN + 1.0f, result.pulseFreq);
}

// =============================================================================
// Test: Stall warning (rapid HIGH tone)
// =============================================================================

void test_stall_warning_at_threshold(void) {
    // At stall warning AOA - maximum pulse rate
    ToneResult result = computeTone(FLAPS_UP.stallWarnAOA, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::High, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, HIGH_TONE_STALL_PPS, result.pulseFreq);
}

void test_stall_warning_above_threshold(void) {
    // Well above stall warning - still maximum pulse
    ToneResult result = computeTone(20.0f, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::High, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, HIGH_TONE_STALL_PPS, result.pulseFreq);
}

void test_stall_warning_extreme_aoa(void) {
    // Extreme AOA (fully stalled) - still works
    ToneResult result = computeTone(30.0f, NORMAL_IAS, MUTE_IAS, FLAPS_UP);
    TEST_ASSERT_EQUAL(ToneType::High, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, HIGH_TONE_STALL_PPS, result.pulseFreq);
}

// =============================================================================
// Test: Different flap configurations
// =============================================================================

void test_flaps_full_lower_thresholds(void) {
    // Full flaps has lower AOA thresholds
    // AOA of 6.0 is on-speed with full flaps but slow with flaps up
    ToneResult result = computeTone(6.0f, NORMAL_IAS, MUTE_IAS, FLAPS_FULL);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.pulseFreq);  // On speed
}

void test_flaps_full_stall_earlier(void) {
    // Full flaps stalls at lower AOA
    ToneResult result = computeTone(12.0f, NORMAL_IAS, MUTE_IAS, FLAPS_FULL);
    TEST_ASSERT_EQUAL(ToneType::High, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, HIGH_TONE_STALL_PPS, result.pulseFreq);
}

// =============================================================================
// Test: Edge case where L/D max >= onSpeedFast (skips pulsed low region)
// =============================================================================

void test_edge_ldmax_above_onspeed_fast(void) {
    // When ldMaxAOA >= onSpeedFastAOA, pulsed low region is skipped
    // AOA of 4.0 is below both, should be no tone
    ToneResult result = computeTone(4.0f, NORMAL_IAS, MUTE_IAS, FLAPS_EDGE);
    TEST_ASSERT_EQUAL(ToneType::None, result.tone);
}

void test_edge_jumps_to_onspeed(void) {
    // Should go directly from no tone to steady low
    ToneResult result = computeTone(5.5f, NORMAL_IAS, MUTE_IAS, FLAPS_EDGE);
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.pulseFreq);
}

// =============================================================================
// Test: Realistic flight scenarios
// =============================================================================

void test_scenario_takeoff_climb(void) {
    // Takeoff climb: moderate AOA, high IAS
    ToneResult result = computeTone(6.0f, 100.0f, MUTE_IAS, FLAPS_UP);
    // Below on-speed, above L/D max - pulsed low
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_GREATER_THAN(0.0f, result.pulseFreq);
}

void test_scenario_pattern_entry(void) {
    // Pattern entry: slowing down, AOA increasing
    ToneResult result = computeTone(7.5f, 90.0f, MUTE_IAS, FLAPS_UP);
    // On speed
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.pulseFreq);
}

void test_scenario_final_approach(void) {
    // Final approach with full flaps
    ToneResult result = computeTone(6.5f, 70.0f, MUTE_IAS, FLAPS_FULL);
    // On speed for landing
    TEST_ASSERT_EQUAL(ToneType::Low, result.tone);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.pulseFreq);
}

void test_scenario_slow_on_final(void) {
    // Getting slow on final - warning tone
    ToneResult result = computeTone(10.0f, 60.0f, MUTE_IAS, FLAPS_FULL);
    // High tone warns of slow condition
    TEST_ASSERT_EQUAL(ToneType::High, result.tone);
    TEST_ASSERT_GREATER_THAN(HIGH_TONE_PPS_MIN, result.pulseFreq);
}

// =============================================================================
// Main test runner
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Audio muting
    RUN_TEST(test_tone_muted_below_mute_ias);
    RUN_TEST(test_tone_muted_at_exactly_mute_ias);
    RUN_TEST(test_tone_active_above_mute_ias);

    // No tone region
    RUN_TEST(test_no_tone_below_ldmax);
    RUN_TEST(test_no_tone_at_zero_aoa);
    RUN_TEST(test_no_tone_negative_aoa);

    // Pulsed LOW tone
    RUN_TEST(test_pulsed_low_at_ldmax);
    RUN_TEST(test_pulsed_low_approaches_onspeed);
    RUN_TEST(test_pulsed_low_midpoint);

    // Steady LOW tone (on speed)
    RUN_TEST(test_steady_low_at_onspeed_fast);
    RUN_TEST(test_steady_low_at_onspeed_slow);
    RUN_TEST(test_steady_low_middle_of_onspeed);

    // Pulsed HIGH tone
    RUN_TEST(test_pulsed_high_just_above_onspeed_slow);
    RUN_TEST(test_pulsed_high_midpoint);
    RUN_TEST(test_pulsed_high_approaching_stall);

    // Stall warning
    RUN_TEST(test_stall_warning_at_threshold);
    RUN_TEST(test_stall_warning_above_threshold);
    RUN_TEST(test_stall_warning_extreme_aoa);

    // Different flap configs
    RUN_TEST(test_flaps_full_lower_thresholds);
    RUN_TEST(test_flaps_full_stall_earlier);

    // Edge cases
    RUN_TEST(test_edge_ldmax_above_onspeed_fast);
    RUN_TEST(test_edge_jumps_to_onspeed);

    // Realistic scenarios
    RUN_TEST(test_scenario_takeoff_climb);
    RUN_TEST(test_scenario_pattern_entry);
    RUN_TEST(test_scenario_final_approach);
    RUN_TEST(test_scenario_slow_on_final);

    return UNITY_END();
}
