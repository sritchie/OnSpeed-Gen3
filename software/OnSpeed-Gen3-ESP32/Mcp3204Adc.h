#ifndef MCP3204_ADC_H
#define MCP3204_ADC_H

#include <Arduino.h>

// Reads 12-bit counts (0..4095) from a Microchip MCP3204 (single-ended).
// Uses the existing sensor SPI bus (SCLK/MOSI/MISO) and a dedicated CS pin.
//
// Only available on HW_V4P builds.
#ifdef HW_V4P
uint16_t Mcp3204Read(uint8_t channel);
#endif

#endif

