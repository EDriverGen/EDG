#include "mcp23017.h"
#include "bus_i2c.h"
#include "dev_i2c.h"
#include "transform.h"
#include "bus.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "bus_pin.h"
#define MCP23017_ADDR 0x20
#define IODIRA 0x00
#define IODIRB 0x01
#define GPIOA 0x12
#define GPIOB 0x13

static int i2c_write(struct mcp23017_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    uint8_t buf[2];
    if (len > 1) return -EINVAL;
    buf[0] = reg;
    if (len == 1) buf[1] = data[0];
    int ret = PrivWrite(dev->fd, buf, len + 1);
    if (ret < 0) return -EIO;
    return 0;
}

static int i2c_write_then_read(struct mcp23017_dev *dev, uint8_t reg, uint8_t *rx, uint16_t rx_len)
{
    uint8_t tx = reg;
    int ret = PrivWrite(dev->fd, &tx, 1);
    if (ret < 0) return -EIO;
    ret = PrivRead(dev->fd, rx, rx_len);
    if (ret < 0) return -EIO;
    return 0;
}

int mcp23017_init(struct mcp23017_dev *dev, struct I2cBus *bus)
{
    (void)bus;
    dev->bus = bus;
    dev->addr = MCP23017_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return -EIO;

    uint16_t i2c_addr = dev->addr;
    struct PrivIoctlCfg ioctl_cfg;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        return -EIO;
    }

    uint8_t data = 0x00;
    ret = i2c_write(dev, IODIRA, &data, 1);
    if (ret < 0) {
        PrivClose(dev->fd);
        return ret;
    }

    ret = i2c_write(dev, IODIRB, &data, 1);
    if (ret < 0) {
        PrivClose(dev->fd);
        return ret;
    }

    return 0;
}

int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta)
{
    return i2c_write_then_read(dev, GPIOA, porta, 1);
}

int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb)
{
    return i2c_write_then_read(dev, GPIOB, portb, 1);
}
