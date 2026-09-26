#include "vl53l0x.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <nuttx/i2c/i2c_master.h>
#include "arch.h"

#define VL53L0X_I2C_ADDR 0x52
#define VL53L0X_REG_RESULT_RANGE_STATUS 0x51

static int vl53l0x_read_reg16(struct vl53l0x_dev_s *dev, uint8_t reg, uint16_t *val)
{
    struct i2c_config_s config;
    config.frequency = 400000;
    config.address = VL53L0X_I2C_ADDR;
    config.addrlen = 7;

    int ret = I2C_WRITE(dev->bus, &config, &reg, 1);
    if (ret < 0) {
        return -EIO;
    }

    uint8_t buf[2];
    ret = I2C_READ(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return -EIO;
    }

    *val = ((uint16_t)buf[0] << 8) | buf[1];
    return 0;
}

int vl53l0x_init(struct vl53l0x_dev_s *dev, struct i2c_master_s *bus)
{
    if (!dev || !bus) {
        return -EINVAL;
    }
    dev->bus = bus;
    dev->addr = VL53L0X_I2C_ADDR;

    up_mdelay(2);

    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_dev_s *dev, int32_t *raw)
{
    if (!dev || !raw) {
        return -EINVAL;
    }

    uint16_t val;
    int ret = vl53l0x_read_reg16(dev, VL53L0X_REG_RESULT_RANGE_STATUS, &val);
    if (ret < 0) {
        return ret;
    }

    *raw = (int32_t)val;
    return 0;
}