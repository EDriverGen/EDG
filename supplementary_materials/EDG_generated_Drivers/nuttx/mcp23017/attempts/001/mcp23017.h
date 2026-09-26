#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include <stdbool.h>
#include <nuttx/i2c/i2c_master.h>

#define MCP23017_I2C_ADDR 0x20

struct i2c_master_s;

struct mcp23017_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int mcp23017_init(struct mcp23017_dev_s *dev, struct i2c_master_s *bus);
int mcp23017_read_gpioa(struct mcp23017_dev_s *dev, uint8_t *porta);
int mcp23017_read_gpiob(struct mcp23017_dev_s *dev, uint8_t *portb);

#endif /* MCP23017_H */