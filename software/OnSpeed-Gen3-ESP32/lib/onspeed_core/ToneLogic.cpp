/*
 * AOA Tone Logic - Implementation
 *
 * This is a pure function extraction of the tone selection logic from Audio.cpp
 * for testability. The logic determines what audio tone to play based on AOA.
 */

#include "ToneLogic.h"

// Linear interpolation helper
// Use a unique name to avoid conflicts with mapfloat in native_compat.h or Helpers.cpp
static float toneMapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
    if ((in_max - in_min) + out_min == 0)
        return 0;
    else
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

ToneResult computeTone(float aoa, float ias, float muteUnderIAS, const FlapAOAConfig& config) {
    ToneResult result;
    result.tone = ToneType::None;
    result.pulseFreq = 0.0f;

    // If airspeed is low (like taxiing) don't make audio
    if (ias <= muteUnderIAS) {
        result.pulseFreq = 20.0f;  // Keep update rate high for quick response
        return result;
    }

    // Check AOA value and set tone and pulse rate accordingly
    // Note: These thresholds are checked from highest (most critical) to lowest

    if (aoa >= config.stallWarnAOA) {
        // STALL WARNING: play 20 pps HIGH tone
        result.tone = ToneType::High;
        result.pulseFreq = HIGH_TONE_STALL_PPS;
    }
    else if (aoa > config.onSpeedSlowAOA) {
        // SLOW (approaching stall): play HIGH tone at 1.5-6.2 PPS
        result.tone = ToneType::High;
        result.pulseFreq = toneMapfloat(
            aoa,
            config.onSpeedSlowAOA,
            config.stallWarnAOA,
            HIGH_TONE_PPS_MIN,
            HIGH_TONE_PPS_MAX);
    }
    else if (aoa >= config.onSpeedFastAOA) {
        // ON SPEED (optimal): play a steady LOW tone
        result.tone = ToneType::Low;
        result.pulseFreq = 0.0f;  // Continuous tone
    }
    else if (aoa >= config.ldMaxAOA &&
             config.ldMaxAOA < config.onSpeedFastAOA) {
        // FAST (above L/D max but below on-speed): play LOW tone at 1.5-8.2 PPS
        // Note: Skip this if L/D max >= onSpeedFast (happens with full flaps)
        result.tone = ToneType::Low;
        result.pulseFreq = toneMapfloat(
            aoa,
            config.ldMaxAOA,
            config.onSpeedFastAOA,
            LOW_TONE_PPS_MIN,
            LOW_TONE_PPS_MAX);
    }
    // else: Below L/D max - no tone (too fast)

    return result;
}
