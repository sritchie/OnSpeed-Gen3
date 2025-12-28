// Core type definitions for OnSpeed algorithms
// Platform-independent, no Arduino dependencies

#pragma once
#include <cstdint>

constexpr int MAX_CURVE_COEFF = 4;  // 4 coefficients = 3rd degree polynomial

struct SuCalibrationCurve {
    float   afCoeff[MAX_CURVE_COEFF];
    uint8_t iCurveType;  // 1=polynomial, 2=logarithmic, 3=exponential
};
