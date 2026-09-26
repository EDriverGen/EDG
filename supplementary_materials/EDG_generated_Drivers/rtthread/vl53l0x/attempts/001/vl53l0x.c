#include "vl53l0x.h"
#include <stdint.h>
#include <string.h>

#define VL53L0X_I2C_ADDR 0x29

int vl53l0x_init(struct vl53l0x_device *dev, struct rt_i2c_bus_device *bus)
{
    if (dev == NULL || bus == NULL) {
        return -1;
    }
    dev->bus = bus;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_device *dev, uint16_t *raw)
{
    struct rt_i2c_msg msgs[1];
    uint8_t buf[2];
    int ret;

    if (dev == NULL || dev->bus == NULL || raw == NULL) {
        return -1;
    }

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = RT_I2C_RD;
    msgs[0].len = 2;
    msgs[0].buf = buf;

    ret = rt_i2c_transfer(dev->bus, msgs, 1);
    if (ret != 1) {
        return -1;
    }

    *raw = ((uint16_t)buf[0] << 8) | buf[1];
    return 0;
}
