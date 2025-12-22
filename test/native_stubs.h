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

// Global stubs for logging (the real code uses g_Log extensively)
class FakeMsgLog {
public:
    enum Module { EnMain, EnConfig, EnAHRS, EnDisk, EnIMU };
    enum Level { EnDebug, EnWarning, EnError, EnOff };

    bool Test(Module m, Level l) { (void)m; (void)l; return false; }
    void print(Module m, Level l, const char* s) { (void)m; (void)l; (void)s; }
    void println(Module m, Level l, const char* s) { (void)m; (void)l; (void)s; }
    void printf(Module m, Level l, const char* fmt, ...) { (void)m; (void)l; (void)fmt; }
    void print(const char* s) { (void)s; }
    void println(const char* s) { (void)s; }
    void printf(const char* fmt, ...) { (void)fmt; }
};

// Stubs for global objects used by the code under test
extern FakeMsgLog g_Log;
extern float g_fCoeffP;

// Note: SuCalibrationCurve is now defined in lib/onspeed_core/CurveCalc.h
// with proper #ifdef NATIVE_BUILD handling

#endif /* __cplusplus */

#endif // NATIVE_BUILD
#endif // NATIVE_STUBS_H
