
#pragma once

// For ESP32: SuCalibrationCurve is defined in Config.h
// For native: SuCalibrationCurve is defined in native_stubs.h (force-included)
#ifndef NATIVE_BUILD
#include "Config.h"
#endif

float CurveCalc(float x, SuCalibrationCurve curve);
