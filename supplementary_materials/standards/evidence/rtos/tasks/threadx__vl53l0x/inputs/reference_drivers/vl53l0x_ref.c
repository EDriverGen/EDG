/*
 * VL53L0X Time-of-Flight Sensor Driver
 * Register 0xC0: MODEL_ID=0xEE, 0x00: start measurement, 0x14+10..11: range mm
 */
#include "vl53l0x_ref.h"


static int vl53l0x_threadx_i2c_write(struct vl53l0x_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int vl53l0x_threadx_i2c_read(struct vl53l0x_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int vl53l0x_threadx_i2c_write_read(struct vl53l0x_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define VL53L0X_I2C_WRITE(_bus, _addr, _data, _len) \
    vl53l0x_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define VL53L0X_I2C_READ(_bus, _addr, _data, _len) \
    vl53l0x_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define VL53L0X_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    vl53l0x_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

static int vl53l0x_read_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return VL53L0X_I2C_WRITE_READ(dev->bus_context, dev->addr, &reg, 1, buf, len);
}
static int vl53l0x_write_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return VL53L0X_I2C_WRITE(dev->bus_context, dev->addr, buf, 2);
}

int vl53l0x_init(struct vl53l0x_device *dev, void *bus_context, const struct vl53l0x_i2c_ops *ops, uint16_t addr) {
    if (!dev) return -1;
    dev->bus_context = bus_context;
    dev->ops = ops; dev->addr = addr;
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
    tx_thread_sleep(5);
    ret = vl53l0x_read_reg(dev, 0x13, &status, 1); if (ret) return ret;
    ret = vl53l0x_read_reg(dev, 0x14, data, 12); if (ret) return ret;
    *range_mm = (uint16_t)((data[10] << 8) | data[11]);
    ret = vl53l0x_write_reg(dev, 0x0B, 0x01); if (ret) return ret;
    return 0;
}
