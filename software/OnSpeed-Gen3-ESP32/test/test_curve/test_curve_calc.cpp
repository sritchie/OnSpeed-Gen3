/*
 * Unit tests for CurveCalc
 *
 * CurveCalc converts pressure coefficient to angle of attack using
 * calibration curves. This is critical for accurate AOA indication.
 *
 * Curve types:
 *   1 = Polynomial: y = a0*x^5 + a1*x^4 + a2*x^3 + a3*x^2 + a4*x + a5
 *   2 = Logarithmic: y = a*ln(x) + b
 *   3 = Exponential: y = a*e^(b*x)
 */

#include <unity.h>
#include <cmath>

// Include from lib/onspeed_core
#include <CurveCalc.h>

void setUp(void) {
    // Nothing to set up
}

void tearDown(void) {
    // Nothing to clean up
}

// =============================================================================
// Test: Polynomial curves (type 1)
// =============================================================================

void test_polynomial_constant(void) {
    // y = 10 (constant)
    SuCalibrationCurve curve;
    curve.iCurveType = 1;
    for (int i = 0; i < MAX_CURVE_COEFF - 1; i++) {
        curve.afCoeff[i] = 0.0f;
    }
    curve.afCoeff[MAX_CURVE_COEFF - 1] = 10.0f;  // constant term

    float result = CurveCalc(5.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, result);

    result = CurveCalc(0.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, result);
}

void test_polynomial_linear(void) {
    // y = 2x + 5
    SuCalibrationCurve curve;
    curve.iCurveType = 1;
    for (int i = 0; i < MAX_CURVE_COEFF; i++) {
        curve.afCoeff[i] = 0.0f;
    }
    curve.afCoeff[MAX_CURVE_COEFF - 2] = 2.0f;   // x coefficient
    curve.afCoeff[MAX_CURVE_COEFF - 1] = 5.0f;   // constant

    // y = 2*3 + 5 = 11
    float result = CurveCalc(3.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 11.0f, result);

    // y = 2*0 + 5 = 5
    result = CurveCalc(0.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, result);
}

void test_polynomial_quadratic(void) {
    // y = x^2 + 2x + 1 = (x+1)^2
    SuCalibrationCurve curve;
    curve.iCurveType = 1;
    for (int i = 0; i < MAX_CURVE_COEFF; i++) {
        curve.afCoeff[i] = 0.0f;
    }
    curve.afCoeff[MAX_CURVE_COEFF - 3] = 1.0f;   // x^2 coefficient
    curve.afCoeff[MAX_CURVE_COEFF - 2] = 2.0f;   // x coefficient
    curve.afCoeff[MAX_CURVE_COEFF - 1] = 1.0f;   // constant

    // y = (2+1)^2 = 9
    float result = CurveCalc(2.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 9.0f, result);

    // y = (0+1)^2 = 1
    result = CurveCalc(0.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result);

    // y = (-1+1)^2 = 0
    result = CurveCalc(-1.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result);
}

// =============================================================================
// Test: Logarithmic curves (type 2)
// =============================================================================

void test_logarithmic_basic(void) {
    // y = 21*ln(x) + 16.45 (example from code comments)
    SuCalibrationCurve curve;
    curve.iCurveType = 2;
    for (int i = 0; i < MAX_CURVE_COEFF; i++) {
        curve.afCoeff[i] = 0.0f;
    }
    curve.afCoeff[MAX_CURVE_COEFF - 2] = 21.0f;    // ln coefficient
    curve.afCoeff[MAX_CURVE_COEFF - 1] = 16.45f;   // constant

    // y = 21*ln(1) + 16.45 = 0 + 16.45 = 16.45
    float result = CurveCalc(1.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 16.45f, result);

    // y = 21*ln(e) + 16.45 = 21 + 16.45 = 37.45
    result = CurveCalc((float)M_E, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 37.45f, result);
}

// =============================================================================
// Test: Exponential curves (type 3)
// =============================================================================

void test_exponential_basic(void) {
    // y = 12.5*e^(-1.63x) (example from code comments)
    SuCalibrationCurve curve;
    curve.iCurveType = 3;
    for (int i = 0; i < MAX_CURVE_COEFF; i++) {
        curve.afCoeff[i] = 0.0f;
    }
    curve.afCoeff[MAX_CURVE_COEFF - 2] = 12.5f;   // multiplier
    curve.afCoeff[MAX_CURVE_COEFF - 1] = -1.63f;  // exponent coefficient

    // y = 12.5*e^0 = 12.5
    float result = CurveCalc(0.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.5f, result);

    // y = 12.5*e^(-1.63) ≈ 12.5 * 0.196 ≈ 2.45
    result = CurveCalc(1.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 2.45f, result);
}

// =============================================================================
// Test: Edge cases
// =============================================================================

void test_polynomial_with_zero_input(void) {
    // y = x^3 + x^2 + x + 1
    SuCalibrationCurve curve;
    curve.iCurveType = 1;
    for (int i = 0; i < MAX_CURVE_COEFF; i++) {
        curve.afCoeff[i] = 0.0f;
    }
    curve.afCoeff[MAX_CURVE_COEFF - 4] = 1.0f;  // x^3
    curve.afCoeff[MAX_CURVE_COEFF - 3] = 1.0f;  // x^2
    curve.afCoeff[MAX_CURVE_COEFF - 2] = 1.0f;  // x
    curve.afCoeff[MAX_CURVE_COEFF - 1] = 1.0f;  // constant

    // y = 0 + 0 + 0 + 1 = 1
    float result = CurveCalc(0.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result);
}

void test_polynomial_with_negative_input(void) {
    // y = x^2 (always positive)
    SuCalibrationCurve curve;
    curve.iCurveType = 1;
    for (int i = 0; i < MAX_CURVE_COEFF; i++) {
        curve.afCoeff[i] = 0.0f;
    }
    curve.afCoeff[MAX_CURVE_COEFF - 3] = 1.0f;  // x^2 coefficient

    // y = (-3)^2 = 9
    float result = CurveCalc(-3.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 9.0f, result);
}

void test_unknown_curve_type_returns_zero(void) {
    SuCalibrationCurve curve;
    curve.iCurveType = 99;  // Invalid type
    for (int i = 0; i < MAX_CURVE_COEFF; i++) {
        curve.afCoeff[i] = 1.0f;
    }

    // Should return 0 for unknown curve type
    float result = CurveCalc(5.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result);
}

// =============================================================================
// Test: Realistic AOA calibration curve
// =============================================================================

void test_realistic_aoa_calibration(void) {
    // A typical AOA calibration might be a 3rd order polynomial
    // that maps pressure coefficient (0.3 to 2.0) to AOA (0 to 20 degrees)
    SuCalibrationCurve curve;
    curve.iCurveType = 1;
    for (int i = 0; i < MAX_CURVE_COEFF; i++) {
        curve.afCoeff[i] = 0.0f;
    }
    // Coefficients for: AOA = 10*Cp (simple linear for testing)
    curve.afCoeff[MAX_CURVE_COEFF - 2] = 10.0f;

    // Pressure coefficient 1.0 -> AOA 10 degrees
    float result = CurveCalc(1.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.0f, result);

    // Pressure coefficient 1.5 -> AOA 15 degrees
    result = CurveCalc(1.5f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 15.0f, result);
}

// =============================================================================
// Main test runner
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Polynomial tests
    RUN_TEST(test_polynomial_constant);
    RUN_TEST(test_polynomial_linear);
    RUN_TEST(test_polynomial_quadratic);

    // Logarithmic tests
    RUN_TEST(test_logarithmic_basic);

    // Exponential tests
    RUN_TEST(test_exponential_basic);

    // Edge cases
    RUN_TEST(test_polynomial_with_zero_input);
    RUN_TEST(test_polynomial_with_negative_input);
    RUN_TEST(test_unknown_curve_type_returns_zero);

    // Realistic scenario
    RUN_TEST(test_realistic_aoa_calibration);

    return UNITY_END();
}
