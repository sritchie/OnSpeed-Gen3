// CurveCalc - Calibration curve calculation for OnSpeed
// Platform-independent, no Arduino dependencies

#pragma once
#include "OnSpeedTypes.h"

// Calculate an output value based on one of the defined curves.
// This is used to calculate AOA from coefficient of pressure
// but could be used for any calibration curve.
//
// Curve types:
//   1 = Polynomial (3rd degree): y = a*x^3 + b*x^2 + c*x + d
//   2 = Logarithmic: y = a*log(x) + b
//   3 = Exponential: y = a*e^(b*x)
//
// Returns 0 for unknown curve types.
float CurveCalc(float x, SuCalibrationCurve curve);
