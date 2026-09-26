#include "mcp23017.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

static int i2c_write(struct mcp23017_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    uint8_t buf[256];
    buf[0] = reg;
    if (len > 0) {
        memcpy(&buf[1], data, len);
    }
    return PrivWrite(dev->fd, buf, 1 + len);
}

static int i2c_read(struct mcp23017_dev *dev, uint8_t *data, uint16_t len)
{
    return PrivRead(dev->fd, data, len);
}

int mcp23017_init(struct mcp23017_dev *dev, struct I2cBus *bus)
{
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = MCP23017_I2C_ADDR;

    dev->bus = bus;
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) {
        return -EIO;
    }

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -EIO;
    }

    uint8_t iodira_data = 0x00;
    ret = i2c_write(dev, MCP23017_IODIRA, &iodira_data, 1);
    if (ret < 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -EIO;
    }

    uint8_t iodirb_data = 0x00;
    ret = i2c_write(dev, MCP23017_IODIRB, &iodirb_data, 1);
    if (ret < 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -EIO;
    }

    return 0;
}

int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta)
{
    int ret;
    uint8_t reg = MCP23017_GPIOA;

    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) {
        return -EIO;
    }

    ret = PrivRead(dev->fd, porta, 1);
    if (ret < 0) {
        return -EIO;
    }

    return 0;
}

int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb)
{
    int ret;
    uint8_t reg = MCP23017_GPIOB;

    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) {
        return -EIO;
    }

    ret = PrivRead(dev->fd, portb, 1);
    if (ret < 0) {
        return -EIO;
    }

    return 0;
}
