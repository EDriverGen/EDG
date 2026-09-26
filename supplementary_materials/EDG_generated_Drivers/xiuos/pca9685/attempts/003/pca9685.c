#include "pca9685.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define PCA9685_ALLCALL_ADDR_READ 0x71

static int i2c_write_read(struct pca9685_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    int ret;

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret != 0) return ret;

    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret != 1) return -EIO;

    ret = PrivRead(dev->fd, buf, len);
    if (ret != len) return -EIO;

    return 0;
}

int pca9685_init(struct pca9685_device *dev, struct I2cBus *bus_handle)
{
    dev->bus = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return -EIO;

    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret != 0) {
        PrivClose(dev->fd);
        return ret;
    }

    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *value)
{
    if (channel > 1) return -EINVAL;
    uint8_t reg = PCA9685_LED0_OFF_L + channel * 4;
    uint8_t buf[4];
    int ret;

    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = PCA9685_ALLCALL_ADDR_READ;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret != 0) return ret;

    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret != 1) return -EIO;

    ret = PrivRead(dev->fd, buf, 4);
    if (ret != 4) return -EIO;

    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    uint16_t duty = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    *value = duty;

    return 0;
}
