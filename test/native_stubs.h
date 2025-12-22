/*
 * Native build stubs for unit testing
 *
 * This header provides minimal stubs for Arduino/ESP32 functions
 * so that pure math/logic code can be compiled and tested on the host machine.
 */

#ifndef NATIVE_STUBS_H
#define NATIVE_STUBS_H

#ifdef NATIVE_BUILD

/* Only include C++ headers when compiling C++ code */
#ifdef __cplusplus

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Arduino types
typedef bool boolean;

// Arduino math functions that might not exist on all platforms
#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

#ifndef abs
#define abs(x) ((x) > 0 ? (x) : -(x))
#endif

// Stub for Arduino.h
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }
inline void delay(unsigned long ms) { (void)ms; }
inline void delayMicroseconds(unsigned int us) { (void)us; }

// Stub for Serial
class FakeSerial {
public:
    void begin(unsigned long baud) { (void)baud; }
    void print(const char* s) { printf("%s", s); }
    void println(const char* s) { printf("%s\n", s); }
    void printf(const char* fmt, ...) { (void)fmt; }
    int available() { return 0; }
    int read() { return -1; }
    void flush() {}
};

// Stub for MsgLog - matches the interface in src/ErrorLogger.h
class MsgLog {
public:
    MsgLog() {}

    // Logging levels - must match ErrorLogger.h
    enum EnLevel { EnDebug, EnWarning, EnError, EnOff };

    // Modules - must match ErrorLogger.h
    enum EnModule {
        EnMain, EnAHRS, EnAudio, EnBoom, EnConfig, EnWebServer,
        EnDataServer, EnDisplay, EnEfis, EnPressure, EnIMU,
        EnReplay, EnDisk, EnSensors, EnSwitch, EnVolume, EnVN300,
        ModuleCount
    };

    // All methods are no-ops for testing
    void Set(EnModule, EnLevel) {}
    bool Set(const char*, EnLevel) { return true; }
    bool Test(EnModule, EnLevel) { return false; }
    char* szLevelName(EnLevel) { return nullptr; }

    void print(EnModule, EnLevel, const char*) {}
    void println(EnModule, EnLevel, const char*) {}
    void printf(EnModule, EnLevel, const char*, ...) {}

    size_t print(const char*) { return 0; }
    size_t println(const char*) { return 0; }
    size_t printf(const char*, ...) { return 0; }

    void flush() {}
};

// Stubs for global objects used by the code under test
extern MsgLog g_Log;
extern float g_fCoeffP;

// Note: SuCalibrationCurve is now defined in lib/onspeed_core/CurveCalc.h
// with proper #ifdef NATIVE_BUILD handling

#endif /* __cplusplus */

#endif // NATIVE_BUILD
#endif // NATIVE_STUBS_H
