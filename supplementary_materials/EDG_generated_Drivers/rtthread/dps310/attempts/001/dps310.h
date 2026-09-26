#ifndef DPS310_H
#define DPS310_H

#include <stdint.h>
#include <rtthread.h>

struct dps310_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int dps310_init(struct dps310_device *dev, struct rt_i2c_bus_device *bus);
int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_raw);
int dps310_read_temperature(struct dps310_device *dev, int32_t *temperature_mdegc);

#endif /* DPS310_H */