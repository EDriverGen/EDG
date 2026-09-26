#include "vl53l0x.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <string.h>

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
    return PrivWrite(dev->fd, &reg, 1);
}

static int i2c_read_regs(struct vl53l0x_dev *dev, uint8_t reg, uint8_t *buf, size_t len) {
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) return ret;
    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) return ret;
    return PrivRead(dev->fd, buf, len);
}

int vl53l0x_init(struct vl53l0x_dev *dev, struct I2cBus *bus_handle) {
    if (!dev || !bus_handle) return -EINVAL;
    memset(dev, 0, sizeof(*dev));
    dev->bus = bus_handle;
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
        dev->fd = -1;
        return ret;
    }
    PrivTaskDelay(2);
    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw) {
    if (!dev || !raw) return -EINVAL;
    uint8_t reg = 0x00;
    uint8_t buf[2];
    int ret = i2c_write_reg(dev, 0xC0);
    if (ret < 0) return ret;
    ret = i2c_read_regs(dev, 0xC0, buf, 2);
    if (ret < 0) return ret;
    uint16_t distance = ((uint16_t)buf[0] << 8) | buf[1];
    *raw = (int32_t)distance;
    return 0;
}
