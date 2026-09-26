#ifndef MCP3008_APACHE_MYNEWT_REF_H
#define MCP3008_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_spi.h"
#include <stdint.h>

#define MCP3008_SPI_MAX_HZ 3600000U
#define MCP3008_CHANNELS   8U
#define MCP3008_MAX_VALUE  1023U
#define MCP3008_SINGLE     1U
#define MCP3008_DIFF       0U

struct mcp3008_device {
    int spi_num;
    uint16_t vref_mv;
};

int mcp3008_init(struct mcp3008_device *dev, int spi_num, uint16_t vref_mv);
int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw);
int mcp3008_read_voltage(struct mcp3008_device *dev, uint8_t channel, uint16_t *mv);
int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv);

#endif
