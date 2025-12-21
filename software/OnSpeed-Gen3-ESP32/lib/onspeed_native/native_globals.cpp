/*
 * Native build global variable definitions
 *
 * This file instantiates all the global objects needed by the OnSpeed
 * code when running in native (non-ESP32) mode.
 */

#ifdef NATIVE_BUILD

#include "native_compat.h"

// Semaphores (unused in native mode)
SemaphoreHandle_t xWriteMutex = nullptr;
SemaphoreHandle_t xSensorMutex = nullptr;
SemaphoreHandle_t xSerialLogMutex = nullptr;

// Core globals
MsgLog g_Log;
SdFileSys g_SdFileSys;
SensorIO g_Sensors;
AHRS g_AHRS;
Flaps g_Flaps;
FOSConfig g_Config;
AudioPlay g_AudioPlay;

// IMU pointer
static IMU330 s_IMU;
IMU330* g_pIMU = &s_IMU;

// Misc globals
volatile double g_fCoeffP = 0;
volatile int g_iDataMark = 0;

#endif // NATIVE_BUILD
