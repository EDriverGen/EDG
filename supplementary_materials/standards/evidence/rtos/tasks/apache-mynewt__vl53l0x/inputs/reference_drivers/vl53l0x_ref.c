#include "vl53l0x_ref.h"

static int vl53l0x_read_reg(struct vl53l0x_device *dev, uint8_t reg,
                            uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data xfer;
    if (dev == 0 || buf == 0 || len == 0) {
        return -1;
    }
    xfer.address = (uint8_t)dev->addr;
    xfer.len = 1;
    xfer.buffer = &reg;
    if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 0) != 0) {
        return -1;
    }
    xfer.address = (uint8_t)dev->addr;
    xfer.len = len;
    xfer.buffer = buf;
    return hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0
        ? 0 : -1;
}

static int vl53l0x_write_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t val)
{
    struct hal_i2c_master_data xfer;
    uint8_t frame[2] = {reg, val};
    if (dev == 0) {
        return -1;
    }
    xfer.address = (uint8_t)dev->addr;
    xfer.len = sizeof(frame);
    xfer.buffer = frame;
    return hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0
        ? 0 : -1;
}

int vl53l0x_init(struct vl53l0x_device *dev, uint8_t i2c_num, uint16_t addr)
{
    if (dev == 0 || addr == 0) {
        return -1;
    }
    if (hal_i2c_init(i2c_num, 0) != 0) {
        return -1;
    }
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    return 0;
}

int vl53l0x_probe(struct vl53l0x_device *dev)
{
    uint8_t id = 0;
    if (vl53l0x_read_reg(dev, 0xC0, &id, 1) != 0) {
        return -1;
    }
    return id == VL53L0X_MODEL_ID ? 0 : -3;
}

int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm)
{
    uint8_t status = 0;
    uint8_t data[12];
    if (dev == 0 || range_mm == 0) {
        return -1;
    }
    if (vl53l0x_write_reg(dev, 0x00, 0x01) != 0) {
        return -1;
    }
    os_time_delay(os_time_ms_to_ticks32(50));
    if (vl53l0x_read_reg(dev, 0x13, &status, 1) != 0) {
        return -1;
    }
    (void)status;
    if (vl53l0x_read_reg(dev, 0x14, data, sizeof(data)) != 0) {
        return -1;
    }
    *range_mm = (uint16_t)(((uint16_t)data[10] << 8) | data[11]);
    return vl53l0x_write_reg(dev, 0x0B, 0x01);
}
