#ifndef MCP3008_H
#define MCP3008_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "cmsis_rtx.h"
struct mcp3008_dev {
    SPI_HandleTypeDef *hspi;
};

void mcp3008_init(struct mcp3008_dev *dev, void *bus_handle);
int mcp3008_read_channel(struct mcp3008_dev *dev, uint8_t channel, uint16_t *out);

#endif
