#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define MCP23017_I2C_ADDR 0x20

#include "freertos.h"
struct mcp23017_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int mcp23017_init(struct mcp23017_dev *dev, void *bus_handle);
int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_val);
int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_val);

#endif
