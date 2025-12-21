
#pragma once

#ifdef NATIVE_BUILD
// For native builds, define the minimal structure locally
// (unless already defined by native_compat.h)
#ifndef MAX_CURVE_COEFF
#define MAX_CURVE_COEFF 6
#endif

#ifndef SUCALIBRATIONCURVE_DEFINED
#define SUCALIBRATIONCURVE_DEFINED
struct SuCalibrationCurve {
    int iCurveType;         // 1=polynomial, 2=logarithmic, 3=exponential
    float afCoeff[MAX_CURVE_COEFF];
};
#endif
#else
// For ESP32 builds, use the full Config.h
#include "Config.h"
#endif

float CurveCalc(float x, SuCalibrationCurve curve);
