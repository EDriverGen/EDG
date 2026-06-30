#include "mcp23017_ref.h"
#include <errno.h>

static int _read_regs(FAR struct mcp23017_device *dev, uint8_t reg,
                      FAR uint8_t *buf, int len)
{
    if (!dev || !dev->i2c || !buf) return -EINVAL;
    return i2c_writeread(dev->i2c, &dev->config, &reg, 1, buf, len);
}

int mcp23017_init(FAR struct mcp23017_device *dev,
                  FAR struct i2c_master_s *i2c, uint8_t addr)
{
    if (!dev || !i2c) return -EINVAL;
    dev->i2c = i2c;
    dev->config.frequency = MCP23017_I2C_FREQ;
    dev->config.address = addr;
    dev->config.addrlen = 7;
    return 0;
}

int mcp23017_read_ports(FAR struct mcp23017_device *dev,
                         uint8_t *porta, uint8_t *portb)
{
    uint8_t buf[2];
    if (_read_regs(dev, MCP23017_REG_GPIOA, buf, 2) < 0)
        return -EIO;
    *porta = buf[0];
    *portb = buf[1];
    return 0;
}

#define _PIN_READ_A(dev, bit) do { \
    uint8_t pa, pb; \
    if (mcp23017_read_ports(dev, &pa, &pb) != 0) return -1; \
    return (pa >> bit) & 1; \
} while(0)

#define _PIN_READ_B(dev, bit) do { \
    uint8_t pa, pb; \
    if (mcp23017_read_ports(dev, &pa, &pb) != 0) return -1; \
    return (pb >> bit) & 1; \
} while(0)

int mcp23017_read_gpa0(FAR struct mcp23017_device *dev) { _PIN_READ_A(dev, 0); }
int mcp23017_read_gpa1(FAR struct mcp23017_device *dev) { _PIN_READ_A(dev, 1); }
int mcp23017_read_gpa2(FAR struct mcp23017_device *dev) { _PIN_READ_A(dev, 2); }
int mcp23017_read_gpa3(FAR struct mcp23017_device *dev) { _PIN_READ_A(dev, 3); }
int mcp23017_read_gpa4(FAR struct mcp23017_device *dev) { _PIN_READ_A(dev, 4); }
int mcp23017_read_gpa5(FAR struct mcp23017_device *dev) { _PIN_READ_A(dev, 5); }
int mcp23017_read_gpa6(FAR struct mcp23017_device *dev) { _PIN_READ_A(dev, 6); }
int mcp23017_read_gpa7(FAR struct mcp23017_device *dev) { _PIN_READ_A(dev, 7); }
int mcp23017_read_gpb0(FAR struct mcp23017_device *dev) { _PIN_READ_B(dev, 0); }
int mcp23017_read_gpb1(FAR struct mcp23017_device *dev) { _PIN_READ_B(dev, 1); }
int mcp23017_read_gpb2(FAR struct mcp23017_device *dev) { _PIN_READ_B(dev, 2); }
int mcp23017_read_gpb3(FAR struct mcp23017_device *dev) { _PIN_READ_B(dev, 3); }
int mcp23017_read_gpb4(FAR struct mcp23017_device *dev) { _PIN_READ_B(dev, 4); }
int mcp23017_read_gpb5(FAR struct mcp23017_device *dev) { _PIN_READ_B(dev, 5); }
int mcp23017_read_gpb6(FAR struct mcp23017_device *dev) { _PIN_READ_B(dev, 6); }
int mcp23017_read_gpb7(FAR struct mcp23017_device *dev) { _PIN_READ_B(dev, 7); }
