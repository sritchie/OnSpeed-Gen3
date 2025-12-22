/*
 * Pressure Calculation Functions
 *
 * Pure math functions for converting pressure sensor counts to engineering units.
 * Extracted from HscPressureSensor.cpp for testability.
 */

#ifndef PRESSURECALC_H
#define PRESSURECALC_H

// Honeywell HSC sensor constants
// These represent the 10%-90% output range of the 12-bit ADC

// HSCDRNN1.6BASA3 - Differential pressure sensor (AOA/Pitot)
constexpr unsigned COUNTS_MIN_DIFF = 409;   // 10% of 4095
constexpr unsigned COUNTS_MAX_DIFF = 3686;  // 90% of 4095
constexpr float PRESSURE_MIN_DIFF = -1.0f;  // PSI
constexpr float PRESSURE_MAX_DIFF = 1.0f;   // PSI

// HSCDRRN100MDSA3 - Absolute pressure sensor (Static)
constexpr unsigned COUNTS_MIN_ABS = 409;
constexpr unsigned COUNTS_MAX_ABS = 3686;
constexpr float PRESSURE_MIN_ABS = 0.0f;    // PSI
constexpr float PRESSURE_MAX_ABS = 23.2f;   // PSI (1.6 bar)

// Unit conversions
constexpr float PSI_TO_MBAR = 68.9476f;
constexpr float MBAR_TO_INHG = 0.02953f;

/**
 * Convert sensor counts to pressure in PSI.
 *
 * @param counts Raw 12-bit ADC counts from sensor
 * @param countsMin Minimum counts (10% = 409 for HSC)
 * @param countsMax Maximum counts (90% = 3686 for HSC)
 * @param pressureMin Pressure at minimum counts
 * @param pressureMax Pressure at maximum counts
 * @return Pressure in PSI
 */
inline float countsToPSI(unsigned counts, unsigned countsMin, unsigned countsMax,
                         float pressureMin, float pressureMax) {
    return (float(counts) - float(countsMin)) * (pressureMax - pressureMin) /
           float(countsMax - countsMin) + pressureMin;
}

/**
 * Convert pressure from PSI to millibars.
 */
inline float psiToMillibars(float psi) {
    return psi * PSI_TO_MBAR;
}

/**
 * Convert pressure from millibars to inches of mercury.
 */
inline float millibarsToInHg(float mbar) {
    return mbar * MBAR_TO_INHG;
}

/**
 * Convert differential pressure counts to PSI (AOA/Pitot sensors).
 */
inline float diffCountsToPSI(unsigned counts) {
    return countsToPSI(counts, COUNTS_MIN_DIFF, COUNTS_MAX_DIFF,
                       PRESSURE_MIN_DIFF, PRESSURE_MAX_DIFF);
}

/**
 * Convert absolute pressure counts to PSI (Static sensor).
 */
inline float absCountsToPSI(unsigned counts) {
    return countsToPSI(counts, COUNTS_MIN_ABS, COUNTS_MAX_ABS,
                       PRESSURE_MIN_ABS, PRESSURE_MAX_ABS);
}

/**
 * Convert absolute pressure counts to millibars (Static sensor).
 */
inline float absCountsToMillibars(unsigned counts) {
    return psiToMillibars(absCountsToPSI(counts));
}

#endif // PRESSURECALC_H
