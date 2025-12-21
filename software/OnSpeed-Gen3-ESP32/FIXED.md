# Bugs Fixed

This document tracks bugs discovered and fixed in the OnSpeed Gen3 codebase.

---

## 1. KalmanFilter constrain() not assigned

**File:** `lib/onspeed_core/KalmanFilter.cpp:66`

**Bug:** The `constrain()` macro result was not being assigned, so the line had no effect:
```cpp
// BEFORE (bug):
zAccelVariance_ = fabs(accel)/50.0f;
constrain(zAccelVariance_, 1.0f, 50.0f);  // Does nothing!

// AFTER (fixed):
zAccelVariance_ = fabs(accel)/50.0f;
zAccelVariance_ = constrain(zAccelVariance_, 1.0f, 50.0f);
```

**Impact:** The acceleration variance was not being clamped to [1.0, 50.0] range, which could cause the Kalman filter to behave unexpectedly with very small or large acceleration values.

**Found by:** Unit test failure in `test_kalman_converges_to_constant_input`

**Fixed:** 2024-12-20

---

## 2. SdFileSys::Init() checking void return value

**File:** `src/SdFileSys.cpp:52`

**Bug:** Arduino ESP32 Core 3.x changed `SPIClass::begin()` to return `void` instead of `bool`. The code was checking the return value:
```cpp
// BEFORE (incompatible with Core 3.x):
if (!uSD_SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS))
    return false;

// AFTER (fixed):
// Note: SPIClass::begin() returns void in Arduino Core 3.x
uSD_SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
```

**Impact:** Code would not compile with Arduino ESP32 Core 3.x.

**Fixed:** 2024-12-20

---

## 3. ConsoleSerial using wrong type for USB CDC

**File:** `src/ConsoleSerial.h:22`

**Bug:** On ESP32-S3 with USB CDC enabled, `Serial` is type `HWCDC*` not `HardwareSerial*`. The code was using the wrong type:
```cpp
// BEFORE (incompatible with ESP32-S3 USB CDC):
HardwareSerial    * pSerial;

// AFTER (fixed - uses common base class):
// Use Stream* for compatibility with both HardwareSerial (ESP32)
// and HWCDC (ESP32-S3 USB CDC) in Arduino Core 3.x
Stream            * pSerial;
```

**Impact:** Code would not compile for ESP32-S3 with USB CDC on boot enabled.

**Fixed:** 2024-12-20

---

## 4. Audio.cpp using unavailable C++23 header

**File:** `src/Audio.cpp:21`

**Bug:** The code included `<bit>` header for `std::byteswap`, but this is C++23 and not available in the ESP32 toolchain:
```cpp
// BEFORE (C++23, not available):
#include <bit>
// ... later ...
i2s.write(std::byteswap(iLeftValue));

// AFTER (fixed - portable implementation in Audio.h):
inline int16_t byteswap16(int16_t val) {
    return (int16_t)(((uint16_t)val >> 8) | ((uint16_t)val << 8));
}
```

**Impact:** Code would not compile. Note: The actual usage was already commented out with a manual byte-swap, but the include was still present.

**Fixed:** 2024-12-20

---

## 5. Config.h missing standard includes

**File:** `src/Config.h:6-7`

**Bug:** The header used `uint8_t` and `String` without including the necessary headers, relying on implicit inclusion from other files:
```cpp
// BEFORE (missing includes):
#include <vector>
// ... later uses uint8_t and String without defining them

// AFTER (fixed):
#include <cstdint>  // For uint8_t, etc.
#include <vector>

#ifdef ARDUINO
#include <Arduino.h>  // For String type
#else
#include <string>
using String = std::string;
#endif
```

**Impact:** Compilation would fail when `Config.h` was included before `Arduino.h`, particularly in the `onspeed_core` library build.

**Fixed:** 2024-12-20

---

## 6. EFIS type comparison using assignment instead of equality

**File:** `src/LogSensor.cpp:165, 239`

**Bug:** The code used `=` (assignment) instead of `==` (comparison) when checking the EFIS serial type. This would always assign `EnVN300` to `enType`, then evaluate as true:
```cpp
// BEFORE (bug - always true, overwrites enType!):
if (g_EfisSerial.enType = EfisSerialIO::EnVN300)  // VN-300 type data
    m_hLogFile.write(",vnAngularRateRoll,...");
else
    m_hLogFile.write(",efisIAS,...");  // Never reached!

// AFTER (fixed):
if (g_EfisSerial.enType == EfisSerialIO::EnVN300)  // VN-300 type data
```

**Impact:**
1. The EFIS type was being corrupted to always be `EnVN300`
2. Non-VN300 EFIS data (Dynon, GRT, etc.) was never logged with the correct column headers
3. The else branch for standard EFIS logging was never executed

**Locations:**
- Line 165: Log file header writing
- Line 239: Log file data writing

**Found by:** Compiler warning `-Wparentheses` ("suggest parentheses around assignment used as truth value")

**Fixed:** 2024-12-20

---

## 7. Compiler Warning Fixes (December 2024)

Multiple compiler warnings were fixed to improve code quality and prevent potential issues:

### 7.1 `_GNU_SOURCE` Redefinition Warning

**Files:** `src/Globals.h`, `src/SdFileSys.h`

**Fix:** Added `#ifndef _GNU_SOURCE` guards to prevent redefinition when already defined by the compiler.

### 7.2 Uninitialized Variable Warnings

**Files:** `src/Volume.cpp`, `src/ConsoleSerial.cpp`, `src/ConfigWebServer.cpp`, `src/DisplaySerial.cpp`, `src/ErrorLogger.cpp`

**Fix:** Initialized variables that could be used before assignment:
- `iVolPos = 0` (Volume.cpp:14)
- `cSerialCmdChar = 0` (ConsoleSerial.cpp:102)
- `bStatus = false` (ConsoleSerial.cpp:330)
- `iVolPos = 0` (ConsoleSerial.cpp:467)
- `bFormatStatus = false` (ConfigWebServer.cpp:1863)
- `bListStatus = false` (ConfigWebServer.cpp:2239)
- `iLen = 0` (ConfigWebServer.cpp:2378)
- `SerialCRC = 0` (DisplaySerial.cpp:65)
- `iChars = 0` (ErrorLogger.cpp:148, 162, 176)

### 7.3 Deprecated Binary Literal Warnings

**File:** `src/IMU330.cpp`

**Fix:** Changed deprecated `B01010000` notation to standard C++ `0b01010000` (8 occurrences in Init() and Reset() functions).

### 7.4 String Literal to `char*` Conversion Warnings

**Files:** `src/ErrorLogger.h`, `src/ErrorLogger.cpp`

**Fix:** Changed `char*` to `const char*` for string literal compatibility:
- `SuLogInfo::szDescription` member
- `szLevelName()` return type

### 7.5 Catch-by-Value Warnings

**File:** `src/LogReplay.cpp`

**Fix:** Changed `catch (const std::invalid_argument)` to `catch (const std::invalid_argument&)` (17 occurrences). Catching by value unnecessarily copies the exception object.

**Fixed:** 2024-12-20

---

## 8. Additional Compiler Warning Fixes (December 2024)

A second round of warning fixes to eliminate all project-level compiler warnings.

### 8.1 Unused Variable Cleanup

**Files and Fixes:**
- `src/ConfigWebServer.cpp:2030` - Removed unused `contentLength` variable (was calculated but never used since content is sent in chunks)
- `src/ConfigWebServer.cpp:2268` - Removed unused `filesize_char` buffer (formatted but never used, `sFormatBytes()` is used instead)
- `src/IMU330.cpp:395` - Removed unused `result` variable in `GetGyroForAxis()`
- `src/LogReplay.cpp:160` - Removed unused `lTimestamp` variable (timestamp parsed but not used)
- `src/LogReplay.cpp:321` - Removed unused `fFlapRawValueLow` and `fFlapRawValueHigh` variables
- `src/SensorIO.cpp:111` - Removed unused `iFlapsIndex` variable
- `src/OnSpeed-Gen3-ESP32.ino:285` - Removed unused `bLoopStats` variable
- `src/Switch.cpp:20-21` - Removed unused `xWasDelayed` and `xLastWakeTime` variables
- `src/ConsoleSerial.cpp:103` - Removed unused `szListfileFileName` buffer

### 8.2 Format Specifier Fixes

**Files and Fixes:**
- `src/LogSensor.cpp:256` - Changed `%ul` to `%lu` for `unsigned long` timestamp (typo: was `%ul` which means "unsigned int followed by literal 'l'")
- `src/ConfigWebServer.cpp:2317` - Fixed `\&` escape sequence to `&` (backslash before ampersand is unnecessary and warned)

### 8.3 WebSocket Switch Case Completeness

**File:** `src/DataServer.cpp:102-110`

**Fix:** Added missing `WStype_PING` and `WStype_PONG` cases to switch statement to silence `-Wswitch` warning.

### 8.4 Deprecated API Usage

**File:** `src/Switch.cpp:24`

**Fix:** Changed `setPressTicks(1000)` to `setPressMs(1000)` to use the non-deprecated OneButton API.

### 8.5 Member Initialization Order

**File:** `src/SensorIO.cpp:76-83`

**Fix:** Reordered member initializer list to match declaration order in `SensorIO.h`. C++ initializes members in declaration order regardless of initializer list order, so the list should match to avoid surprises.

```cpp
// BEFORE (wrong order):
SensorIO::SensorIO()
    : P45Median(...),    // Declared after PfwdMedian
      PfwdMedian(...),
      ...

// AFTER (correct order matching header):
SensorIO::SensorIO()
    : PfwdMedian(...),   // Matches declaration order
      PfwdAvg(...),
      P45Median(...),
      ...
```

### 8.6 Volatile Increment Deprecated

**File:** `src/Switch.cpp:46`

**Fix:** Changed `g_iDataMark++` to `g_iDataMark = g_iDataMark + 1` because `++` on volatile-qualified types is deprecated in C++20.

### 8.7 Char Array Subscript Warning

**File:** `src/ConsoleSerial.cpp:595-598`

**Fix:** Cast `char` to `unsigned char` when using as array subscript in Base64 decode function. Using signed `char` as array index can cause issues with negative values.

```cpp
// BEFORE (warns about char subscript):
from_base64[sEncodedString[i+0]]

// AFTER (explicit unsigned cast):
from_base64[(unsigned char)sEncodedString[i+0]]
```

### 8.8 Buffer Overflow Prevention

**File:** `src/LogSensor.cpp:121`

**Fix:** Increased `szSensorLogFilename` buffer from 14 to 20 bytes to prevent potential overflow. The format `log_%03d.csv` could theoretically produce up to 15 characters with large file numbers, plus null terminator.

**Fixed:** 2024-12-20

---

## Remaining Warnings (External Libraries)

The following warnings remain but are from external libraries and should not be modified:

- **SdFat library:** `#warning File not defined because __has_include(FS.h)` - Harmless library warning
- **FILE_READ/FILE_WRITE redefinition:** Conflict between Arduino FS.h and SdFat - library compatibility issue
- **OneWire library:** Various warnings about `#undef` directives and type comparisons

These library warnings do not affect functionality and would require modifying external dependencies to fix.

---

## Notes

These bugs were discovered during the migration to PlatformIO with Arduino ESP32 Core 3.x support. The original code was developed with Arduino IDE which may have had different include ordering or used an older ESP32 core version.
