/*
 * AOA Tone Logic - Pure function for determining audio tones based on AOA
 *
 * This extracts the core tone selection logic from Audio.cpp for unit testing.
 * The actual audio playback remains in Audio.cpp; this just computes what
 * tone/pulse should be playing given the current flight state.
 */

#ifndef TONELOGIC_H
#define TONELOGIC_H

// Tone types (matches EnAudioTone in Audio.h)
enum class ToneType {
    None = 0,
    Low = 1,    // 400 Hz - "on speed" / approaching optimal
    High = 2    // 1600 Hz - slow / stall warning
};

// Result of tone logic computation
struct ToneResult {
    ToneType tone;
    float pulseFreq;  // PPS (pulses per second), 0 = continuous tone
};

// Flap configuration for AOA thresholds
struct FlapAOAConfig {
    float ldMaxAOA;        // L/D max AOA (best glide)
    float onSpeedFastAOA;  // Start of "on speed" range (fast side)
    float onSpeedSlowAOA;  // End of "on speed" range (slow side)
    float stallWarnAOA;    // Stall warning AOA
};

// Constants from Audio.cpp
constexpr float HIGH_TONE_STALL_PPS = 20.0f;
constexpr float HIGH_TONE_PPS_MAX = 6.2f;
constexpr float HIGH_TONE_PPS_MIN = 1.5f;
constexpr float LOW_TONE_PPS_MAX = 8.2f;
constexpr float LOW_TONE_PPS_MIN = 1.5f;

/**
 * Compute what tone should be playing based on AOA and configuration.
 *
 * @param aoa Current angle of attack in degrees
 * @param ias Current indicated airspeed
 * @param muteUnderIAS Mute audio below this IAS (e.g., taxiing)
 * @param config Flap position AOA thresholds
 * @return ToneResult with tone type and pulse frequency
 */
ToneResult computeTone(float aoa, float ias, float muteUnderIAS, const FlapAOAConfig& config);

#endif // TONELOGIC_H
