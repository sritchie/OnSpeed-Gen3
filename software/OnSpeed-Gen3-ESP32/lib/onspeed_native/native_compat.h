/*
 * Native build compatibility layer for OnSpeed
 *
 * This header provides stubs and minimal implementations to allow
 * the OnSpeed code to compile and run on the host machine for testing
 * and development without ESP32 hardware.
 *
 * Usage: Included via -include in platformio.ini for native builds
 */

#ifndef NATIVE_COMPAT_H
#define NATIVE_COMPAT_H

#ifdef NATIVE_BUILD

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>

// ============================================================================
// Arduino core compatibility
// ============================================================================

typedef bool boolean;
typedef std::string String;

#define PROGMEM
#define O_READ 0

#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

#ifndef abs
#define abs(x) ((x) > 0 ? (x) : -(x))
#endif

// Time functions with real timing
inline unsigned long millis() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

inline unsigned long micros() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}

inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void delayMicroseconds(unsigned int us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

// ============================================================================
// FreeRTOS compatibility
// ============================================================================

typedef int BaseType_t;
typedef unsigned int TickType_t;
typedef void* SemaphoreHandle_t;
typedef void* TaskHandle_t;
typedef void* RingbufHandle_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdMS_TO_TICKS(ms) (ms)

inline TickType_t xTaskGetTickCount() { return millis(); }

inline BaseType_t xTaskDelayUntil(TickType_t* pxPreviousWakeTime, TickType_t xTimeIncrement) {
    unsigned long now = millis();
    unsigned long target = *pxPreviousWakeTime + xTimeIncrement;
    if (now < target) {
        delay(target - now);
    }
    *pxPreviousWakeTime = millis();
    return pdTRUE;
}

inline BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t) { return pdTRUE; }
inline void xSemaphoreGive(SemaphoreHandle_t) {}

#define xLAST_TICK_TIME(period_msec) ((xTaskGetTickCount() / pdMS_TO_TICKS(period_msec)) * pdMS_TO_TICKS(period_msec))

// Global semaphores (unused in native mode but need to exist)
extern SemaphoreHandle_t xWriteMutex;
extern SemaphoreHandle_t xSensorMutex;
extern SemaphoreHandle_t xSerialLogMutex;

// ============================================================================
// SdFat compatibility - uses std::ifstream
// ============================================================================

class FsFile {
public:
    std::ifstream stream;
    std::string filepath;

    bool isOpen() { return stream.is_open(); }

    void close() {
        if (stream.is_open()) {
            stream.close();
        }
    }

    int fgets(char* buf, size_t len) {
        if (!stream.is_open() || stream.eof()) return 0;
        stream.getline(buf, len);
        if (stream.fail() && !stream.eof()) {
            stream.clear();
            return 0;
        }
        return strlen(buf);
    }
};

class SdFileSys {
public:
    bool bSdAvailable = true;

    bool exists(const char* path) {
        std::ifstream f(path);
        return f.good();
    }

    bool exists(const std::string& path) {
        return exists(path.c_str());
    }

    FsFile open(const char* path, int mode) {
        (void)mode;
        FsFile f;
        f.filepath = path;
        f.stream.open(path);
        return f;
    }
};

extern SdFileSys g_SdFileSys;

// ============================================================================
// Logging compatibility
// ============================================================================

class MsgLog {
public:
    enum Module { EnMain, EnConfig, EnAHRS, EnDisk, EnIMU, EnReplay, EnDataServer };
    enum Level { EnDebug, EnWarning, EnError, EnOff };

    bool Test(Module m, Level l) { (void)m; (void)l; return false; }

    void print(const char* s) { fprintf(stderr, "%s", s); }
    void println(const char* s) { fprintf(stderr, "%s\n", s); }
    void print(Module m, Level l, const char* s) { (void)m; (void)l; fprintf(stderr, "%s", s); }
    void println(Module m, Level l, const char* s) { (void)m; (void)l; fprintf(stderr, "%s\n", s); }

    void printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }

    void printf(Module m, Level l, const char* fmt, ...) {
        (void)m; (void)l;
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
};

extern MsgLog g_Log;

// ============================================================================
// IMU stub
// ============================================================================

struct IMU330 {
    float Ax = 0, Ay = 0, Az = 1.0;  // 1G vertical
    float Gx = 0, Gy = 0, Gz = 0;    // No rotation
};

extern IMU330* g_pIMU;

// ============================================================================
// Sensor structures
// ============================================================================

struct SensorIO {
    float PfwdSmoothed = 0;
    float P45Smoothed = 0;
    float Palt = 0;
    float IAS = 0;
    float AOA = 0;
    float fDecelRate = 0;
};

extern SensorIO g_Sensors;

// ============================================================================
// AHRS structure
// ============================================================================

struct AHRS {
    float SmoothedPitch = 0;
    float SmoothedRoll = 0;
    float FlightPath = 0;
    float KalmanVSI = 0;       // m/s
    float KalmanAlt = 0;       // meters
    float AccelLatCorr = 0;
    float AccelVertCorr = 0;
    float gPitch = 0;          // pitch rate
    float fTAS = 0;            // true airspeed m/s
};

extern AHRS g_AHRS;

// ============================================================================
// Flaps structure
// ============================================================================

struct Flaps {
    int iPosition = 0;
    int iIndex = 0;

    // Stub for TestPotTask (not used in replay, but needs to compile)
    float Read() { return 0.0f; }
};

extern Flaps g_Flaps;

// ============================================================================
// Configuration (Native Build Version)
// ============================================================================
//
// NOTE: This is a simplified config parser for native builds. It intentionally
// does NOT include the production Config.h because:
//
// 1. Config.h redefines SuCalibrationCurve with MAX_CURVE_COEFF=4, conflicting
//    with CurveCalc.h which uses MAX_CURVE_COEFF=6
// 2. Config.h uses Arduino String methods that would require extensive stubbing
// 3. Config.cpp has dependencies on SD card, LittleFS, tinyxml2, and other
//    ESP32-specific libraries
//
// This native version parses only the config values needed for replay/interactive
// mode, using the same file format as the production code. The parsing logic
// mirrors Config.cpp::LoadConfigFromString() for V1 CONFIG format.
//
// TODO: Refactor production Config.h to use the shared SuCalibrationCurve from
// CurveCalc.h instead of defining its own.

// Define SuCalibrationCurve here since it's needed before CurveCalc.h is included.
// This matches the definition in lib/onspeed_core/CurveCalc.h for NATIVE_BUILD.
#ifndef MAX_CURVE_COEFF
#define MAX_CURVE_COEFF 6
#endif

#ifndef SUCALIBRATIONCURVE_DEFINED
#define SUCALIBRATIONCURVE_DEFINED
struct SuCalibrationCurve {
    int iCurveType;  // 1=polynomial, 2=logarithmic, 3=exponential
    float afCoeff[MAX_CURVE_COEFF];
};
#endif

struct SuFlaps {
    int iDegrees = 0;
    int iPotPosition = 0;
    float fLDMAXAOA = 3.0f;
    float fONSPEEDFASTAOA = 6.0f;
    float fONSPEEDSLOWAOA = 8.0f;
    float fSTALLWARNAOA = 12.0f;
    float fSTALLAOA = 15.0f;
    float fMANAOA = 20.0f;
    SuCalibrationCurve AoaCurve;
};

struct FOSConfig {
    std::vector<SuFlaps> aFlaps;
    int iAoaSmoothing = 5;
    int iPressureSmoothing = 15;

    FOSConfig() {
        initDefaultFlaps();
    }

    void initDefaultFlaps() {
        aFlaps.clear();
        SuFlaps flaps0;
        flaps0.iDegrees = 0;
        flaps0.AoaCurve.iCurveType = 1;
        flaps0.AoaCurve.afCoeff[MAX_CURVE_COEFF - 2] = 10.0f;  // Default: AOA = 10 * CP
        aFlaps.push_back(flaps0);
    }

    // Parse value between XML tags: <TAG>value</TAG>
    // Mirrors Config.cpp::GetConfigValue()
    static std::string getConfigValue(const std::string& config, const std::string& tag) {
        std::string startTag = "<" + tag + ">";
        std::string endTag = "</" + tag + ">";
        size_t start = config.find(startTag);
        if (start == std::string::npos) return "";
        start += startTag.length();
        size_t end = config.find(endTag, start);
        if (end == std::string::npos) return "";
        return config.substr(start, end - start);
    }

    // Mirrors Config.cpp::ParseFloatCSV()
    static std::vector<float> parseCSVFloats(const std::string& csv) {
        std::vector<float> result;
        std::stringstream ss(csv);
        std::string item;
        while (std::getline(ss, item, ',')) {
            try { result.push_back(std::stof(item)); }
            catch (...) { result.push_back(0.0f); }
        }
        return result;
    }

    // Mirrors Config.cpp::ParseIntCSV()
    static std::vector<int> parseCSVInts(const std::string& csv) {
        std::vector<int> result;
        std::stringstream ss(csv);
        std::string item;
        while (std::getline(ss, item, ',')) {
            try { result.push_back(std::stoi(item)); }
            catch (...) { result.push_back(0); }
        }
        return result;
    }

    // Mirrors Config.cpp::ParseCurveCSV()
    // Format: "coeff0,coeff1,...,curveType"
    static SuCalibrationCurve parseCurveCSV(const std::string& csv) {
        SuCalibrationCurve curve;
        std::memset(&curve, 0, sizeof(curve));

        std::vector<float> values = parseCSVFloats(csv);
        if (values.empty()) return curve;

        // Last value is curve type (1=poly, 2=log, 3=exp)
        curve.iCurveType = static_cast<int>(values.back());

        // Coefficients are right-aligned in afCoeff array
        int numCoeffs = static_cast<int>(values.size()) - 1;
        int startIdx = MAX_CURVE_COEFF - numCoeffs;
        for (int i = 0; i < numCoeffs && startIdx + i < MAX_CURVE_COEFF; i++) {
            curve.afCoeff[startIdx + i] = values[i];
        }

        return curve;
    }

    // Load V1 CONFIG format from file
    // Mirrors Config.cpp::LoadConfigurationFile() + LoadConfigFromString()
    bool loadFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "  [Config] Could not open: " << path << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string config = buffer.str();
        file.close();

        std::cerr << "  [Config] Loading: " << path << std::endl;

        // Parse smoothing values
        std::string val = getConfigValue(config, "AOA_SMOOTHING");
        if (!val.empty()) iAoaSmoothing = std::stoi(val);

        val = getConfigValue(config, "PRESSURE_SMOOTHING");
        if (!val.empty()) iPressureSmoothing = std::stoi(val);

        // Parse flap configuration arrays
        std::vector<int> flapDegrees = parseCSVInts(getConfigValue(config, "FLAPDEGREES"));
        std::vector<int> flapPots = parseCSVInts(getConfigValue(config, "FLAPPOTPOSITIONS"));
        std::vector<float> ldmaxAOA = parseCSVFloats(getConfigValue(config, "SETPOINT_LDMAXAOA"));
        std::vector<float> onspeedFastAOA = parseCSVFloats(getConfigValue(config, "SETPOINT_ONSPEEDFASTAOA"));
        std::vector<float> onspeedSlowAOA = parseCSVFloats(getConfigValue(config, "SETPOINT_ONSPEEDSLOWAOA"));
        std::vector<float> stallWarnAOA = parseCSVFloats(getConfigValue(config, "SETPOINT_STALLWARNAOA"));

        if (flapDegrees.empty()) {
            std::cerr << "  [Config] No FLAPDEGREES found, using defaults" << std::endl;
            return false;
        }

        // Build flap configurations
        aFlaps.clear();
        for (size_t i = 0; i < flapDegrees.size(); i++) {
            SuFlaps flap;
            flap.iDegrees = flapDegrees[i];
            flap.iPotPosition = (i < flapPots.size()) ? flapPots[i] : 0;
            if (i < ldmaxAOA.size()) flap.fLDMAXAOA = ldmaxAOA[i];
            if (i < onspeedFastAOA.size()) flap.fONSPEEDFASTAOA = onspeedFastAOA[i];
            if (i < onspeedSlowAOA.size()) flap.fONSPEEDSLOWAOA = onspeedSlowAOA[i];
            if (i < stallWarnAOA.size()) flap.fSTALLWARNAOA = stallWarnAOA[i];

            // Parse AOA curve for this flap position
            std::string curveStr = getConfigValue(config, "AOA_CURVE_FLAPS" + std::to_string(i));
            if (!curveStr.empty()) {
                flap.AoaCurve = parseCurveCSV(curveStr);
            }

            aFlaps.push_back(flap);

            std::cerr << "  [Config] Flaps " << flap.iDegrees << "°: "
                      << "LDmax=" << flap.fLDMAXAOA
                      << " OnSpd=" << flap.fONSPEEDFASTAOA << "-" << flap.fONSPEEDSLOWAOA
                      << " Warn=" << flap.fSTALLWARNAOA
                      << " Curve=" << flap.AoaCurve.iCurveType << std::endl;
        }

        std::cerr << "  [Config] Loaded " << aFlaps.size() << " flap configurations" << std::endl;
        return true;
    }
};

extern FOSConfig g_Config;

// ============================================================================
// Audio stub
// ============================================================================

class AudioPlay {
public:
    void UpdateTones() {
        // In native mode, could log audio state or output JSON
    }
    void Init() {}
};

extern AudioPlay g_AudioPlay;

// ============================================================================
// Global variables used by LogReplay
// ============================================================================

extern volatile double g_fCoeffP;
extern volatile int g_iDataMark;

// ============================================================================
// Coefficient of pressure formula
// ============================================================================

#define PCOEFF(p_fwd, p_45) ((p_45) / (p_fwd))

// ============================================================================
// Helper functions
// ============================================================================

inline float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ============================================================================
// Forward declarations for functions that need implementation
// ============================================================================

float CalcAOA(float Pfwd, float P45, int iFlapsIndex, int iSmoothing);

// ============================================================================
// Unit conversion macros (from Globals.h)
// ============================================================================

#ifndef M2FT
#define M2FT(m) ((m) * 3.28084f)        // meters to feet
#endif

#ifndef MPS2FPM
#define MPS2FPM(mps) ((mps) * 196.85f)  // m/s to feet per minute
#endif

// ============================================================================
// JSON output for WebSocket/stdout (simplified from DataServer.cpp)
// ============================================================================

inline std::string UpdateLiveDataJson() {
    float fAccelSumSq = g_pIMU->Ax * g_pIMU->Ax +
                        g_pIMU->Ay * g_pIMU->Ay +
                        g_pIMU->Az * g_pIMU->Az;
    float fVerticalGload = std::sqrt(fAccelSumSq);
    fVerticalGload = std::round(fVerticalGload * 10.0f) / 10.0f;

    if (g_pIMU->Az < 0)
        fVerticalGload *= -1;

    const char* szFormat = R"({"AOA":%.2f,"Pitch":%.2f,"Roll":%.2f,"IAS":%.2f,"PAlt":%.2f,"verticalGLoad":%.2f,"lateralGLoad":%.2f,"LDmax":%.2f,"OnspeedFast":%.2f,"OnspeedSlow":%.2f,"OnspeedWarn":%.2f,"flapsPos":%d,"flapIndex":%d,"coeffP":%.4f,"dataMark":%d,"kalmanVSI":%.2f,"flightPath":%.2f,"PitchRate":%.2f,"DecelRate":%.2f})";

    char szReturn[600];
    std::snprintf(szReturn, sizeof(szReturn), szFormat,
        g_Sensors.AOA,
        g_AHRS.SmoothedPitch,
        g_AHRS.SmoothedRoll,
        g_Sensors.IAS,
        M2FT(g_AHRS.KalmanAlt),
        fVerticalGload,
        g_AHRS.AccelLatCorr,
        g_Config.aFlaps[g_Flaps.iIndex].fLDMAXAOA,
        g_Config.aFlaps[g_Flaps.iIndex].fONSPEEDFASTAOA,
        g_Config.aFlaps[g_Flaps.iIndex].fONSPEEDSLOWAOA,
        g_Config.aFlaps[g_Flaps.iIndex].fSTALLWARNAOA,
        g_Flaps.iPosition,
        g_Flaps.iIndex,
        g_fCoeffP,
        g_iDataMark,
        MPS2FPM(g_AHRS.KalmanVSI),
        g_AHRS.FlightPath,
        g_AHRS.gPitch,
        g_Sensors.fDecelRate
    );

    return std::string(szReturn);
}

#endif // NATIVE_BUILD
#endif // NATIVE_COMPAT_H
