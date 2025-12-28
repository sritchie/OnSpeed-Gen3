// CurveCalc.cpp - Calibration curve evaluation implementation

#include "CurveCalc.h"
#include <cmath>

// ============================================================================
// OPTIONAL DEBUG LOGGING
//
// To enable debug output, define ON Debug logging SPEED_LOG_DEBUG before including this file:
//   #define ONSPEED_LOG_DEBUG(fmt, ...) your_logger(fmt, __VA_ARGS__)
//
// By default, debug logging is a no-op.
// ============================================================================

#ifndef ONSPEED_LOG_DEBUG
#define ONSPEED_LOG_DEBUG(...)
#endif

float CurveCalc(float x, const SuCalibrationCurve& curve) {
    float y = 0.0f;

    // Polynomial: y = a3*x^3 + a2*x^2 + a1*x + a0
    // Coefficients stored as [a3, a2, a1, a0]
    if (curve.iCurveType == 1) {
        for (int i = 0; i < MAX_CURVE_COEFF; ++i) {
            int power = MAX_CURVE_COEFF - i - 1;
            y += curve.afCoeff[i] * std::pow(x, power);
            ONSPEED_LOG_DEBUG("%.2f * pow(%.2f, %d) + ", curve.afCoeff[i], x, power);
        }
        ONSPEED_LOG_DEBUG("= %.2f\n", y);
    }
    // Logarithmic: y = a*ln(x) + b
    // Uses last two coefficients: afCoeff[2] = a, afCoeff[3] = b
    else if (curve.iCurveType == 2) {
        y = curve.afCoeff[MAX_CURVE_COEFF - 2] * std::log(x)
          + curve.afCoeff[MAX_CURVE_COEFF - 1];
        ONSPEED_LOG_DEBUG("%.2f * log(%.2f) + %.2f = %.2f\n",
            curve.afCoeff[MAX_CURVE_COEFF - 2], x,
            curve.afCoeff[MAX_CURVE_COEFF - 1], y);
    }
    // Exponential: y = a*e^(b*x)
    // Uses last two coefficients: afCoeff[2] = a, afCoeff[3] = b
    else if (curve.iCurveType == 3) {
        y = curve.afCoeff[MAX_CURVE_COEFF - 2]
          * std::exp(curve.afCoeff[MAX_CURVE_COEFF - 1] * x);
        ONSPEED_LOG_DEBUG("%.2f * exp(%.2f * %.2f) = %.2f\n",
            curve.afCoeff[MAX_CURVE_COEFF - 2],
            curve.afCoeff[MAX_CURVE_COEFF - 1], x, y);
    }
    // Unknown curve type - return 0
    else {
        y = 0.0f;
    }

    return y;
}
