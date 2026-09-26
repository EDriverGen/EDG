#ifndef __MCP23017_REF_H
#define __MCP23017_REF_H

#include <stddef.h>
#include <stdint.h>

#define MCP23017_I2C_ADDR  0x20
#define MCP23017_REG_GPIOA 0x12

struct mcp23017_i2c_ops {
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct mcp23017_device {
    void *bus_context;
    const struct mcp23017_i2c_ops *ops;
    uint16_t addr;
};

int mcp23017_init(struct mcp23017_device *dev, void *bus_context,
                  const struct mcp23017_i2c_ops *ops, uint16_t addr);
int mcp23017_read_ports(struct mcp23017_device *dev, uint8_t *porta, uint8_t *portb);

int mcp23017_read_gpa0(struct mcp23017_device *dev);
int mcp23017_read_gpa1(struct mcp23017_device *dev);
int mcp23017_read_gpa2(struct mcp23017_device *dev);
int mcp23017_read_gpa3(struct mcp23017_device *dev);
int mcp23017_read_gpa4(struct mcp23017_device *dev);
int mcp23017_read_gpa5(struct mcp23017_device *dev);
int mcp23017_read_gpa6(struct mcp23017_device *dev);
int mcp23017_read_gpa7(struct mcp23017_device *dev);
int mcp23017_read_gpb0(struct mcp23017_device *dev);
int mcp23017_read_gpb1(struct mcp23017_device *dev);
int mcp23017_read_gpb2(struct mcp23017_device *dev);
int mcp23017_read_gpb3(struct mcp23017_device *dev);
int mcp23017_read_gpb4(struct mcp23017_device *dev);
int mcp23017_read_gpb5(struct mcp23017_device *dev);
int mcp23017_read_gpb6(struct mcp23017_device *dev);
int mcp23017_read_gpb7(struct mcp23017_device *dev);

#endif
