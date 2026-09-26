#include "bh1750.h"
#include "transform.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_CMD_RESET 0x07



int bh1750_init(struct bh1750_device *dev, void *bus_handle)
{
    (void)bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) {
        return -EIO;
    }

    uint16_t i2c_addr = dev->i2c_addr;
    struct PrivIoctlCfg ioctl_cfg;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -EIO;
    }

    uint8_t cmd = BH1750_CMD_POWER_ON;
    if (PrivWrite(dev->fd, &cmd, 1) != 1) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -EIO;
    }

    return 0;
}

int bh1750_read_illuminance(struct bh1750_device *dev, int32_t *raw)
{
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    if (PrivWrite(dev->fd, &cmd, 1) != 1) {
        return -EIO;
    }

    PrivTaskDelay(180);

    uint8_t buf[2];
    if (PrivRead(dev->fd, buf, 2) != 2) {
        return -EIO;
    }

    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
