/*
 * MCP23017 16-bit I2C GPIO expander reference driver for RT-Thread.
 * All pins configured as input with pull-up by default (POR state).
 * GPIO register (0x12/0x13) read in sequential auto-increment mode.
 */
#ifndef REF_MCP23017_RTTHREAD_H_
#define REF_MCP23017_RTTHREAD_H_
#include <rtthread.h>
#include <rtdevice.h>
#ifdef __cplusplus
extern "C" {
#endif

#define MCP23017_I2C_ADDR     0x20
#define MCP23017_REG_IODIRA    0x00
#define MCP23017_REG_IODIRB    0x01
#define MCP23017_REG_GPPUA     0x0C
#define MCP23017_REG_GPPUB     0x0D
#define MCP23017_REG_GPIOA     0x12
#define MCP23017_REG_GPIOB     0x13

struct mcp23017_device {
    struct rt_i2c_bus_device *bus;
    uint16_t addr;  /* 7-bit I2C address */
};

rt_err_t mcp23017_init(struct mcp23017_device *dev,
                       struct rt_i2c_bus_device *bus, uint16_t addr);
rt_err_t mcp23017_read_ports(struct mcp23017_device *dev,
                              uint8_t *porta, uint8_t *portb);

/* Per-pin read helpers; each returns 0 or 1, or negative on error. */
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

#ifdef __cplusplus
}
#endif
#endif
