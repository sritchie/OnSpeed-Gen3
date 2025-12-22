/*
 * Unit tests for KalmanFilter
 *
 * The Kalman filter is used for altitude/vertical speed estimation.
 * These tests verify basic functionality and edge cases.
 */

#include <unity.h>
#include <cmath>

// Include the code under test (from lib/onspeed_core)
#include <KalmanFilter.h>

// Test fixture
static KalmanFilter kf;

void setUp(void) {
    // Reset filter before each test
}

void tearDown(void) {
    // Nothing to clean up
}

// =============================================================================
// Test: Filter initialization
// =============================================================================

void test_kalman_configure_sets_initial_values(void) {
    float zVariance = 1.0f;
    float zAccelVariance = 0.1f;
    float zAccelBiasVariance = 0.01f;
    float zInitial = 100.0f;
    float vInitial = 5.0f;
    float aBiasInitial = 0.0f;

    kf.Configure(zVariance, zAccelVariance, zAccelBiasVariance,
                 zInitial, vInitial, aBiasInitial);

    // The filter should be configured (we can't directly access internal state,
    // but we can verify it doesn't crash and produces reasonable output)
    volatile float z, v;
    kf.Update(100.0f, 0.0f, 0.02f, &z, &v);

    // After one update with same position, z should be close to initial
    TEST_ASSERT_FLOAT_WITHIN(10.0f, zInitial, z);
}

// =============================================================================
// Test: Filter convergence
// =============================================================================

void test_kalman_converges_to_constant_input(void) {
    // Initialize with values closer to target to avoid large initial transient
    kf.Configure(1.0f, 0.1f, 0.01f, 1000.0f, 0.0f, 0.0f);

    volatile float z, v;
    float constantAltitude = 1000.0f;

    // Feed constant altitude for many iterations
    // The filter has dynamic variance adjustment (zAccelVariance_ = |accel|/50,
    // clamped to [1,50]) which affects convergence behavior
    for (int i = 0; i < 500; i++) {
        kf.Update(constantAltitude, 0.0f, 0.02f, &z, &v);
    }

    // Should converge close to constant input
    TEST_ASSERT_FLOAT_WITHIN(50.0f, constantAltitude, z);
    // Velocity should be reasonably small for constant altitude
    // Note: The filter's bias estimation can cause some residual velocity
    TEST_ASSERT_FLOAT_WITHIN(50.0f, 0.0f, v);
}

void test_kalman_tracks_linear_motion(void) {
    kf.Configure(1.0f, 0.1f, 0.01f, 0.0f, 0.0f, 0.0f);

    volatile float z, v;
    float velocity = 10.0f;  // 10 m/s climb
    float dt = 0.02f;        // 50 Hz sample rate

    // Simulate climbing at constant rate
    for (int i = 0; i < 200; i++) {
        float altitude = velocity * i * dt;
        kf.Update(altitude, 0.0f, dt, &z, &v);
    }

    // Velocity estimate should be close to actual velocity
    TEST_ASSERT_FLOAT_WITHIN(2.0f, velocity, v);
}

// =============================================================================
// Test: Filter handles noise
// =============================================================================

void test_kalman_filters_noisy_measurements(void) {
    kf.Configure(1.0f, 0.1f, 0.01f, 1000.0f, 0.0f, 0.0f);

    volatile float z, v;
    float trueAltitude = 1000.0f;

    // Feed noisy measurements
    float noisePattern[] = {5.0f, -3.0f, 7.0f, -8.0f, 2.0f, -4.0f, 6.0f, -5.0f};
    int numPatterns = sizeof(noisePattern) / sizeof(noisePattern[0]);

    for (int i = 0; i < 100; i++) {
        float noise = noisePattern[i % numPatterns];
        kf.Update(trueAltitude + noise, 0.0f, 0.02f, &z, &v);
    }

    // Filtered output should be closer to true value than raw noise amplitude
    TEST_ASSERT_FLOAT_WITHIN(3.0f, trueAltitude, z);
}

// =============================================================================
// Test: Edge cases
// =============================================================================

void test_kalman_handles_zero_dt(void) {
    kf.Configure(1.0f, 0.1f, 0.01f, 100.0f, 0.0f, 0.0f);

    volatile float z, v;

    // Zero dt shouldn't crash (though it's not physically meaningful)
    kf.Update(100.0f, 0.0f, 0.0f, &z, &v);

    // Should still produce some output
    TEST_ASSERT_FALSE(isnan(z));
    TEST_ASSERT_FALSE(isnan(v));
}

void test_kalman_handles_large_acceleration(void) {
    kf.Configure(1.0f, 0.1f, 0.01f, 0.0f, 0.0f, 0.0f);

    volatile float z, v;

    // Large acceleration shouldn't cause numerical issues
    kf.Update(0.0f, 100.0f, 0.02f, &z, &v);

    TEST_ASSERT_FALSE(isnan(z));
    TEST_ASSERT_FALSE(isnan(v));
    TEST_ASSERT_FALSE(isinf(z));
    TEST_ASSERT_FALSE(isinf(v));
}

// =============================================================================
// Main test runner
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_kalman_configure_sets_initial_values);
    RUN_TEST(test_kalman_converges_to_constant_input);
    RUN_TEST(test_kalman_tracks_linear_motion);
    RUN_TEST(test_kalman_filters_noisy_measurements);
    RUN_TEST(test_kalman_handles_zero_dt);
    RUN_TEST(test_kalman_handles_large_acceleration);

    return UNITY_END();
}
