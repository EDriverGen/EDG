/*
 * MCP3008 10-bit SPI ADC driver for NuttX
 */
#ifndef MCP3008_REF_H
#define MCP3008_REF_H

#include <nuttx/config.h>
#include <nuttx/spi/spi.h>
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
    FAR struct spi_dev_s *spi;
    uint32_t devid;
    uint16_t vref_mv;
};

int mcp3008_init(FAR struct mcp3008_device *dev, FAR struct spi_dev_s *spi,
                 uint32_t devid, uint16_t vref_mv);
int mcp3008_read_raw(FAR struct mcp3008_device *dev, uint8_t channel, uint8_t single, FAR uint16_t *raw);
int mcp3008_read_voltage(FAR struct mcp3008_device *dev, uint8_t channel, FAR uint16_t *mv);
int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv);

#ifdef __cplusplus
}
#endif
#endif
