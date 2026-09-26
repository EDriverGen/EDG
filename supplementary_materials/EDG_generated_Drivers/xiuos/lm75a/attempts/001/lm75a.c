#include "lm75a.h"
#include "transform.h"
#include <stdint.h>

#define LM75A_I2C_ADDR 0x48
#define LM75A_TEMP_REG 0x00

int lm75a_init(struct lm75a_device *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_addr = LM75A_I2C_ADDR;
    dev->fd = PrivOpen("/dev/i2c1", OPE_INT);
    if (dev->fd < 0) {
        return -1;
    }
    uint16_t i2c_addr = dev->i2c_addr;
    struct PrivIoctlCfg ioctl_cfg;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -1;
    }
    PrivTaskDelay(100);
    return 0;
}

int lm75a_read_temp(struct lm75a_device *dev, int32_t *raw) {
    if (dev->fd < 0) {
        return -1;
    }
    uint8_t cmd = LM75A_TEMP_REG;
    if (PrivWrite(dev->fd, &cmd, 1) != 1) {
        return -1;
    }
    uint8_t buf[2];
    if (PrivRead(dev->fd, buf, 2) != 2) {
        return -1;
    }
    int16_t reg = (int16_t)((buf[0] << 8) | buf[1]);
    int32_t raw_val = (reg >> 5) & 0x7FF;
    if (reg & 0x0400) {
        raw_val |= ~0x7FF;
    }
    *raw = raw_val;
    return 0;
}