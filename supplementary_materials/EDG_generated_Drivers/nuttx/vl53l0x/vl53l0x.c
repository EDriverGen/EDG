#include "vl53l0x.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "arch.h"

#define VL53L0X_I2C_ADDR 0x52
#define VL53L0X_READ_REG_0x51 0x51

static int i2c_write_reg(struct i2c_master_s *bus, uint8_t addr, uint8_t reg)
{
    struct i2c_msg_s msg;
    uint8_t buf[1];
    buf[0] = reg;
    msg.frequency = 100000;
    msg.addr = addr;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = 1;
    int ret = I2C_TRANSFER(bus, &msg, 1);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static int i2c_read_reg16(struct i2c_master_s *bus, uint8_t addr, uint8_t reg, uint16_t *value)
{
    struct i2c_msg_s msg[2];
    uint8_t reg_buf[1];
    uint8_t data_buf[2];
    reg_buf[0] = reg;
    msg[0].frequency = 100000;
    msg[0].addr = addr;
    msg[0].flags = 0;
    msg[0].buffer = reg_buf;
    msg[0].length = 1;
    msg[1].frequency = 100000;
    msg[1].addr = addr;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = data_buf;
    msg[1].length = 2;
    int ret = I2C_TRANSFER(bus, msg, 2);
    if (ret < 0) {
        return ret;
    }
    *value = ((uint16_t)data_buf[0] << 8) | data_buf[1];
    return 0;
}

int vl53l0x_init(struct vl53l0x_dev_s *dev, struct i2c_master_s *bus)
{
    if (dev == NULL || bus == NULL) {
        return -EINVAL;
    }
    dev->bus = bus;
    dev->addr = VL53L0X_I2C_ADDR;
    up_mdelay(2);
    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_dev_s *dev, int32_t *raw)
{
    if (dev == NULL || dev->bus == NULL || raw == NULL) {
        return -EINVAL;
    }
    uint16_t value;
    int ret = i2c_read_reg16(dev->bus, dev->addr, VL53L0X_READ_REG_0x51, &value);
    if (ret < 0) {
        return ret;
    }
    *raw = (int32_t)value;
    return 0;
}