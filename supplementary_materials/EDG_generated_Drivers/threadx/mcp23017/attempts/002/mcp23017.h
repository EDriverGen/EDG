#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stddef.h>

#define MCP23017_I2C_ADDR 0x20

#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13
#define MCP23017_OLATA  0x14
#define MCP23017_OLATB  0x15

struct mcp23017_dev {
    void *bus_handle;
    uint8_t i2c_addr;
};

int mcp23017_init(struct mcp23017_dev *dev, void *bus_handle);
int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_byte);
int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_byte);

#endif