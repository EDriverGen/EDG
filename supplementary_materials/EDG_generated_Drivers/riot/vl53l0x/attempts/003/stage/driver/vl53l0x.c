#include "vl53l0x.h"
#include "xtimer.h"
#include <stdint.h>
#include <stddef.h>

#include "riot.h"
#define VL53L0X_I2C_ADDR 0x52
#define VL53L0X_DISTANCE_REG 0x00

int vl53l0x_init(vl53l0x_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = VL53L0X_I2C_ADDR;
    xtimer_msleep(2);
    return 0;
}

int vl53l0x_read_distance(vl53l0x_t *dev, int32_t *raw) {
    uint8_t reg = VL53L0X_DISTANCE_REG;
    uint8_t buf[2];
    int ret;

    ret = i2c_write_bytes(dev->bus, dev->addr, &reg, 1, 0);
    if (ret != 0) return -1;

    ret = i2c_read_bytes(dev->bus, dev->addr, buf, 2, 0);
    if (ret != 0) return -1;

    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
