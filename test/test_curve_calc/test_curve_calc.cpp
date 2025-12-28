// Tests for CurveCalc - Calibration curve calculations
//
// CurveCalc converts pressure coefficients to AOA using calibration curves.
// Curve types:
//   1 = Polynomial (3rd degree): y = a*x^3 + b*x^2 + c*x + d
//   2 = Logarithmic: y = a*log(x) + b
//   3 = Exponential: y = a*e^(b*x)

#include <unity.h>
#include <OnSpeedTypes.h>
#include <CurveCalc.h>
#include <cmath>

void setUp(void) {}
void tearDown(void) {}

// =============================================================================
// Polynomial curve tests (type 1)
// =============================================================================

// y = 2x^3 + 0x^2 + 0x + 5 at x=2 => y = 16 + 5 = 21
void test_polynomial_cubic(void) {
    SuCalibrationCurve curve = {{2.0f, 0.0f, 0.0f, 5.0f}, 1};
    float result = CurveCalc(2.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 21.0f, result);
}

// y = x^2 at x=3 => y = 9
void test_polynomial_quadratic(void) {
    SuCalibrationCurve curve = {{0.0f, 1.0f, 0.0f, 0.0f}, 1};
    float result = CurveCalc(3.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.0f, result);
}

// y = 2x + 3 at x=5 => y = 13
void test_polynomial_linear(void) {
    SuCalibrationCurve curve = {{0.0f, 0.0f, 2.0f, 3.0f}, 1};
    float result = CurveCalc(5.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 13.0f, result);
}

// y = 7 (constant) at x=anything => y = 7
void test_polynomial_constant(void) {
    SuCalibrationCurve curve = {{0.0f, 0.0f, 0.0f, 7.0f}, 1};
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, CurveCalc(0.0f, curve));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, CurveCalc(100.0f, curve));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, CurveCalc(-50.0f, curve));
}

// Full cubic: y = 1*x^3 + 2*x^2 + 3*x + 4 at x=2
// = 8 + 8 + 6 + 4 = 26
void test_polynomial_full_cubic(void) {
    SuCalibrationCurve curve = {{1.0f, 2.0f, 3.0f, 4.0f}, 1};
    float result = CurveCalc(2.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 26.0f, result);
}

// =============================================================================
// Logarithmic curve tests (type 2)
// =============================================================================

// y = 10*log(e) + 5 at x=e => y = 10*1 + 5 = 15
void test_logarithmic_at_e(void) {
    SuCalibrationCurve curve = {{0.0f, 0.0f, 10.0f, 5.0f}, 2};
    float result = CurveCalc(std::exp(1.0f), curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, result);
}

// y = 1*log(1) + 0 = 0
void test_logarithmic_at_one(void) {
    SuCalibrationCurve curve = {{0.0f, 0.0f, 1.0f, 0.0f}, 2};
    float result = CurveCalc(1.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

// y = 2*log(e^3) + 1 = 2*3 + 1 = 7
void test_logarithmic_at_e_cubed(void) {
    SuCalibrationCurve curve = {{0.0f, 0.0f, 2.0f, 1.0f}, 2};
    float result = CurveCalc(std::exp(3.0f), curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.0f, result);
}

// =============================================================================
// Exponential curve tests (type 3)
// =============================================================================

// y = 2*e^(0.5*2) = 2*e^1 ≈ 5.436
void test_exponential_basic(void) {
    SuCalibrationCurve curve = {{0.0f, 0.0f, 2.0f, 0.5f}, 3};
    float expected = 2.0f * std::exp(1.0f);
    float result = CurveCalc(2.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expected, result);
}

// y = 1*e^(1*0) = 1*e^0 = 1
void test_exponential_at_zero(void) {
    SuCalibrationCurve curve = {{0.0f, 0.0f, 1.0f, 1.0f}, 3};
    float result = CurveCalc(0.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, result);
}

// y = 3*e^(-0.5*2) = 3*e^-1 ≈ 1.104
void test_exponential_negative_exponent(void) {
    SuCalibrationCurve curve = {{0.0f, 0.0f, 3.0f, -0.5f}, 3};
    float expected = 3.0f * std::exp(-1.0f);
    float result = CurveCalc(2.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expected, result);
}

// =============================================================================
// Edge cases
// =============================================================================

// Unknown curve type returns 0
void test_unknown_curve_type_returns_zero(void) {
    SuCalibrationCurve curve = {{1.0f, 2.0f, 3.0f, 4.0f}, 99};
    float result = CurveCalc(5.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

// Curve type 0 returns 0
void test_curve_type_zero_returns_zero(void) {
    SuCalibrationCurve curve = {{1.0f, 2.0f, 3.0f, 4.0f}, 0};
    float result = CurveCalc(5.0f, curve);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

// Polynomial with x=0
void test_polynomial_at_zero(void) {
    SuCalibrationCurve curve = {{1.0f, 2.0f, 3.0f, 4.0f}, 1};
    float result = CurveCalc(0.0f, curve);
    // 1*0^3 + 2*0^2 + 3*0 + 4 = 4
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, result);
}

// Polynomial with negative x
void test_polynomial_negative_x(void) {
    SuCalibrationCurve curve = {{1.0f, 0.0f, 0.0f, 0.0f}, 1};
    float result = CurveCalc(-2.0f, curve);
    // 1*(-2)^3 = -8
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -8.0f, result);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Polynomial tests
    RUN_TEST(test_polynomial_cubic);
    RUN_TEST(test_polynomial_quadratic);
    RUN_TEST(test_polynomial_linear);
    RUN_TEST(test_polynomial_constant);
    RUN_TEST(test_polynomial_full_cubic);

    // Logarithmic tests
    RUN_TEST(test_logarithmic_at_e);
    RUN_TEST(test_logarithmic_at_one);
    RUN_TEST(test_logarithmic_at_e_cubed);

    // Exponential tests
    RUN_TEST(test_exponential_basic);
    RUN_TEST(test_exponential_at_zero);
    RUN_TEST(test_exponential_negative_exponent);

    // Edge cases
    RUN_TEST(test_unknown_curve_type_returns_zero);
    RUN_TEST(test_curve_type_zero_returns_zero);
    RUN_TEST(test_polynomial_at_zero);
    RUN_TEST(test_polynomial_negative_x);

    return UNITY_END();
}
