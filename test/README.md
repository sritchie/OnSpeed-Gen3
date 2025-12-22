# OnSpeed Gen3 ESP32 Unit Tests

This directory contains unit tests for the OnSpeed firmware that can run on your development machine (not requiring ESP32 hardware).

## Running Tests

### Using PlatformIO CLI

```bash
# Run all tests
pio test -e native

# Run with verbose output
pio test -e native -v

# Run specific test file
pio test -e native --filter test_kalman_filter
```

### Using PlatformIO IDE

1. Open the project in VS Code with PlatformIO extension
2. Click the PlatformIO icon in the sidebar
3. Expand "native" environment
4. Click "Test"

## Test Structure

```
test/
├── README.md                 # This file
├── native_stubs.h           # Stubs for Arduino/ESP32 functions
├── test_stubs.cpp           # Stub implementations
├── test_kalman_filter.cpp   # Kalman filter tests
└── test_curve_calc.cpp      # Calibration curve tests
```

## What's Tested

### KalmanFilter (`test_kalman_filter.cpp`)

The Kalman filter is used for altitude and vertical speed estimation. Tests verify:

- Initialization with various parameters
- Convergence to constant input
- Tracking of linear motion
- Noise filtering behavior
- Edge cases (zero dt, large accelerations)

### CurveCalc (`test_curve_calc.cpp`)

CurveCalc converts pressure coefficient to angle of attack. Tests verify:

- Polynomial curves (type 1) - constant, linear, quadratic
- Logarithmic curves (type 2)
- Exponential curves (type 3)
- Edge cases (negative inputs, unknown curve types)

## Adding New Tests

1. Create a new file named `test_<module>.cpp`
2. Include Unity framework: `#include <unity.h>`
3. Include native stubs if needed: `#include "native_stubs.h"`
4. Write test functions starting with `test_`
5. Add main() with UNITY_BEGIN/END and RUN_TEST macros

Example:

```cpp
#include <unity.h>
#include "native_stubs.h"

void test_something_works(void) {
    TEST_ASSERT_EQUAL(42, calculate_answer());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_something_works);
    return UNITY_END();
}
```

## Limitations

These are **native tests** that run on your computer, not on ESP32 hardware. They can only test:

- Pure math/logic functions
- Algorithms that don't depend on hardware
- Data structures and parsing

They **cannot** test:

- Hardware communication (SPI, I2C, Serial)
- FreeRTOS task behavior
- Real-time performance
- Sensor readings
- WiFi/network functionality

For hardware testing, use the actual device with serial debug output.

## Test Framework

We use [Unity](https://github.com/ThrowTheSwitch/Unity), a lightweight C test framework. Common assertions:

```cpp
TEST_ASSERT_EQUAL(expected, actual)
TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)
TEST_ASSERT_TRUE(condition)
TEST_ASSERT_FALSE(condition)
TEST_ASSERT_NULL(pointer)
TEST_ASSERT_NOT_NULL(pointer)
```
