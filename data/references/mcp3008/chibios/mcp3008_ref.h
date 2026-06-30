/*
 * MCP3008 10-bit SPI ADC driver for ChibiOS
 */
#ifndef MCP3008_REF_H
#define MCP3008_REF_H

#include "hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCP3008_SPI_MAX_HZ   3600000
#define MCP3008_CHANNELS     8
#define MCP3008_MAX_VALUE    1023
#define MCP3008_SINGLE       1
#define MCP3008_DIFF         0

struct mcp3008_device
{
    SPIDriver *spid;
    const SPIConfig *spi_cfg;
    uint16_t vref_mv;
};

int mcp3008_init(struct mcp3008_device *dev, SPIDriver *spid, const SPIConfig *cfg, uint16_t vref_mv);
int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw);
int mcp3008_read_voltage(struct mcp3008_device *dev, uint8_t channel, uint16_t *mv);
int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv);

#ifdef __cplusplus
}
#endif
#endif
