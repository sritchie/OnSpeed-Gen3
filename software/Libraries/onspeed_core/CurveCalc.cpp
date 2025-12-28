// CurveCalc - Calibration curve calculation for OnSpeed
// Platform-independent implementation

#include "CurveCalc.h"
#include <cmath>

// Debug logging is optional - disabled by default
// To enable on ESP32, define LOG_DEBUG before including this file:
//   #define LOG_DEBUG(...) g_Log.printf(MsgLog::EnAHRS, MsgLog::EnDebug, __VA_ARGS__)
#ifndef LOG_DEBUG
#define LOG_DEBUG(...)
#endif

float CurveCalc(float fX, SuCalibrationCurve suCurve)
{
    float fY = 0.0f;

    // Polynomial: y = a*x^3 + b*x^2 + c*x + d
    if (suCurve.iCurveType == 1)
    {
        for (int i = 0; i < MAX_CURVE_COEFF; i++)
        {
            fY += suCurve.afCoeff[i] * pow(fX, MAX_CURVE_COEFF - i - 1);
            LOG_DEBUG("%.2f * pow(%.2f,%i)+", suCurve.afCoeff[i], fX, MAX_CURVE_COEFF - i - 1);
        }
        LOG_DEBUG("=%.2f\n", fY);
    }

    // Logarithmic: y = a*log(x) + b
    else if (suCurve.iCurveType == 2)
    {
        fY = suCurve.afCoeff[MAX_CURVE_COEFF - 2] * log(fX) + suCurve.afCoeff[MAX_CURVE_COEFF - 1];
        LOG_DEBUG("%.2f * log(%.2f)+%.2f\n", suCurve.afCoeff[MAX_CURVE_COEFF - 2], fX, suCurve.afCoeff[MAX_CURVE_COEFF - 1]);
    }

    // Exponential: y = a*e^(b*x)
    else if (suCurve.iCurveType == 3)
    {
        fY = suCurve.afCoeff[MAX_CURVE_COEFF - 2] * exp(suCurve.afCoeff[MAX_CURVE_COEFF - 1] * fX);
    }

    return fY;
}
