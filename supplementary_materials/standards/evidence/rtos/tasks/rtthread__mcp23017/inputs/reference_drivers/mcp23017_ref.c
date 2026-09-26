/*
 * MCP23017 for RT-Thread. Reads GPIOA+GPIOB in one sequential I2C transaction.
 * Per-handbook: set pointer to 0x12, read 2 bytes (GPIOA then GPIOB).
 */
#include "mcp23017_ref.h"

/* Internal: write pointer then read n bytes */
static int _read_regs(struct mcp23017_device *dev, uint8_t reg,
                      uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[2];
    msgs[0].addr  = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;
    msgs[1].addr  = dev->addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buf;
    msgs[1].len   = len;
    return rt_i2c_transfer(dev->bus, msgs, 2) == 2 ? RT_EOK : -RT_ERROR;
}

/* Internal: write register with 1 byte value */
static int _write_reg(struct mcp23017_device *dev, uint8_t reg, uint8_t val)
{
    struct rt_i2c_msg msgs[1];
    uint8_t buf[2] = {reg, val};
    msgs[0].addr  = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buf;
    msgs[0].len   = 2;
    return rt_i2c_transfer(dev->bus, msgs, 1) == 1 ? RT_EOK : -RT_ERROR;
}

rt_err_t mcp23017_init(struct mcp23017_device *dev,
                       struct rt_i2c_bus_device *bus, uint16_t addr)
{
    if (!dev || !bus) return -RT_EINVAL;
    dev->bus  = bus;
    dev->addr = addr;
    /* POR defaults to input; no pull-ups needed for mock testing */
    (void)dev;
    return RT_EOK;
}

rt_err_t mcp23017_read_ports(struct mcp23017_device *dev,
                              uint8_t *porta, uint8_t *portb)
{
    uint8_t buf[2];
    if (_read_regs(dev, MCP23017_REG_GPIOA, buf, 2) != RT_EOK)
        return -RT_ERROR;
    *porta = buf[0];
    *portb = buf[1];
    return RT_EOK;
}

/* Per-pin readers — handbook pin mapping:
   Port A: GPA0=bit0 ... GPA7=bit7
   Port B: GPB0=bit0 ... GPB7=bit7 */

#define _PIN_READ_A(dev, bit) do { \
    uint8_t pa, pb; \
    if (mcp23017_read_ports(dev, &pa, &pb) != RT_EOK) return -1; \
    return (pa >> bit) & 1; \
} while(0)

#define _PIN_READ_B(dev, bit) do { \
    uint8_t pa, pb; \
    if (mcp23017_read_ports(dev, &pa, &pb) != RT_EOK) return -1; \
    return (pb >> bit) & 1; \
} while(0)

int mcp23017_read_gpa0(struct mcp23017_device *dev) { _PIN_READ_A(dev, 0); }
int mcp23017_read_gpa1(struct mcp23017_device *dev) { _PIN_READ_A(dev, 1); }
int mcp23017_read_gpa2(struct mcp23017_device *dev) { _PIN_READ_A(dev, 2); }
int mcp23017_read_gpa3(struct mcp23017_device *dev) { _PIN_READ_A(dev, 3); }
int mcp23017_read_gpa4(struct mcp23017_device *dev) { _PIN_READ_A(dev, 4); }
int mcp23017_read_gpa5(struct mcp23017_device *dev) { _PIN_READ_A(dev, 5); }
int mcp23017_read_gpa6(struct mcp23017_device *dev) { _PIN_READ_A(dev, 6); }
int mcp23017_read_gpa7(struct mcp23017_device *dev) { _PIN_READ_A(dev, 7); }
int mcp23017_read_gpb0(struct mcp23017_device *dev) { _PIN_READ_B(dev, 0); }
int mcp23017_read_gpb1(struct mcp23017_device *dev) { _PIN_READ_B(dev, 1); }
int mcp23017_read_gpb2(struct mcp23017_device *dev) { _PIN_READ_B(dev, 2); }
int mcp23017_read_gpb3(struct mcp23017_device *dev) { _PIN_READ_B(dev, 3); }
int mcp23017_read_gpb4(struct mcp23017_device *dev) { _PIN_READ_B(dev, 4); }
int mcp23017_read_gpb5(struct mcp23017_device *dev) { _PIN_READ_B(dev, 5); }
int mcp23017_read_gpb6(struct mcp23017_device *dev) { _PIN_READ_B(dev, 6); }
int mcp23017_read_gpb7(struct mcp23017_device *dev) { _PIN_READ_B(dev, 7); }
