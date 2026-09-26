#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>
#include <rtdevice.h>

#define BH1750_I2C_ADDR 0x23

typedef struct {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
    uint8_t mt_reg;
} bh1750_device_t;

int bh1750_init(bh1750_device_t *dev, struct rt_i2c_bus_device *bus);
int bh1750_read_illuminance(bh1750_device_t *dev, uint16_t *raw);

#endif
