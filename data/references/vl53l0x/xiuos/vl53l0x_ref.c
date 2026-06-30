/*
 * VL53L0X Time-of-Flight Sensor Driver
 * Register 0xC0: MODEL_ID=0xEE, 0x00: start measurement, 0x14+10..11: range mm
 */
#include "vl53l0x_ref.h"


static int vl53l0x_read_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    if (PrivWrite(dev->fd, &reg, 1) < 0) return -1;
    if (PrivRead(dev->fd, buf, len) < 0) return -1;
    return 0;
}
static int vl53l0x_write_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return (PrivWrite(dev->fd, buf, 2) < 0) ? -1 : 0;
}

int vl53l0x_init(struct vl53l0x_device *dev, const char *i2c_path, uint16_t addr) {
    if (!dev) return -1;
    dev->fd = PrivOpen(i2c_path, O_RDWR);
    if (dev->fd < 0) return -1;
    dev->addr = addr;
    struct PrivIoctlCfg cfg;
    cfg.ioctl_driver_type = I2C_TYPE;
    cfg.args = &addr;
    PrivIoctl(dev->fd, OPE_INT, &cfg);
    return 0;
}

int vl53l0x_probe(struct vl53l0x_device *dev) {
    uint8_t id;
    int ret = vl53l0x_read_reg(dev, 0xC0, &id, 1);
    if (ret) return ret;
    return (id == VL53L0X_MODEL_ID) ? 0 : -3;
}

int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm) {
    uint8_t data[12], status; int ret;
    if (!dev || !range_mm) return -1;
    ret = vl53l0x_write_reg(dev, 0x00, 0x01); if (ret) return ret;
    PrivTaskDelay(50);
    ret = vl53l0x_read_reg(dev, 0x13, &status, 1); if (ret) return ret;
    ret = vl53l0x_read_reg(dev, 0x14, data, 12); if (ret) return ret;
    *range_mm = (uint16_t)((data[10] << 8) | data[11]);
    ret = vl53l0x_write_reg(dev, 0x0B, 0x01); if (ret) return ret;
    return 0;
}
