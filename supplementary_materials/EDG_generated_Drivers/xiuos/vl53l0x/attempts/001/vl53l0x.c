#include "vl53l0x.h"
#include "transform.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "bus.h"
#include "bus_i2c.h"
#include "bus_pin.h"
#define VL53L0X_I2C_ADDR 0x52

static int i2c_write_reg(struct vl53l0x_dev *dev, uint8_t reg) {
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) return ret;
    ret = PrivWrite(dev->fd, &reg, 1);
    return ret;
}

static int i2c_read_regs(struct vl53l0x_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) return ret;
    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) return ret;
    ret = PrivRead(dev->fd, buf, len);
    return ret;
}

int vl53l0x_init(struct vl53l0x_dev *dev, struct I2cBus *bus) {
    if (!dev || !bus) return -EINVAL;
    dev->bus = bus;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return dev->fd;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        return ret;
    }
    PrivTaskDelay(2);
    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw) {
    if (!dev || !raw) return -EINVAL;
    uint8_t reg = 0x00;
    uint8_t buf[2];
    int ret = i2c_read_regs(dev, reg, buf, 2);
    if (ret < 0) return ret;
    uint16_t val = ((uint16_t)buf[0] << 8) | buf[1];
    *raw = (int32_t)val;
    return 0;
}
