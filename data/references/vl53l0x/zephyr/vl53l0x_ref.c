/*
 * VL53L0X Time-of-Flight Sensor Driver
 * Register 0xC0: MODEL_ID=0xEE, 0x00: start measurement, 0x14+10..11: range mm
 */
#include "vl53l0x_ref.h"


static int vl53l0x_read_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}
static int vl53l0x_write_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t val) {
    return i2c_reg_write_byte(dev->bus, dev->addr, reg, val);
}

int vl53l0x_init(struct vl53l0x_device *dev, const struct device * bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
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
    k_msleep(50);
    ret = vl53l0x_read_reg(dev, 0x13, &status, 1); if (ret) return ret;
    ret = vl53l0x_read_reg(dev, 0x14, data, 12); if (ret) return ret;
    *range_mm = (uint16_t)((data[10] << 8) | data[11]);
    ret = vl53l0x_write_reg(dev, 0x0B, 0x01); if (ret) return ret;
    return 0;
}
