#include "Mcp3204Adc.h"
#include "Globals.h"

#ifdef HW_V4P

// MCP3204 timing is very forgiving; keep this conservative to avoid bus issues.
static constexpr uint32_t kMcp3204SpiClockHz = 1000000;

uint16_t Mcp3204Read(uint8_t channel)
{
    if (g_pSensorSPI == nullptr || g_pSensorSPI->pSPI == nullptr)
        return 0;

    channel &= 0x03; // MCP3204 has channels 0..3

    // Command format (MCP3204/3208 family, SPI mode 0):
    //  - Start bit + single-ended + channel select bits packed across 3 bytes.
    // This matches common MCP320x 3-byte transfer implementations.
    uint8_t tx0 = uint8_t(0x06 | (channel >> 2));
    uint8_t tx1 = uint8_t((channel & 0x03) << 6);

    g_pSensorSPI->pSPI->beginTransaction(SPISettings(kMcp3204SpiClockHz, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_ADC, LOW);

    (void)g_pSensorSPI->pSPI->transfer(tx0);
    uint8_t rx1 = g_pSensorSPI->pSPI->transfer(tx1);
    uint8_t rx2 = g_pSensorSPI->pSPI->transfer(0x00);

    digitalWrite(CS_ADC, HIGH);
    g_pSensorSPI->pSPI->endTransaction();

    return uint16_t(((rx1 & 0x0F) << 8) | rx2);
}

#endif
