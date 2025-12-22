#pragma once

#include <Arduino.h>  // For Stream base class

#include "Globals.h"

#define CONSOLE_BUFFER_SIZE    127


class ConsoleSerialIO
{
public:
    ConsoleSerialIO();

    // Structures

    // Data
public:
    // Use Stream* for compatibility with both HardwareSerial (ESP32)
    // and HWCDC (ESP32-S3 USB CDC) in Arduino Core 3.x
    Stream            * pSerial;
    bool                bEnabled;

    char                CmdBuffer[CONSOLE_BUFFER_SIZE];
    byte                CmdBufferIndex;

//    unsigned long       uTimestamp; // Millisecond timestamp of decoded data
//    unsigned long       LastReceivedTime;

    // Methods
public:
    void Init();
    void Enable(bool bEnable);
    void Read();
    void DisplayConsoleHelp();
    void PrintTaskInfo(TaskHandle_t  xTask);

};