#ifndef DPS310_H
#define DPS310_H

#include <stdint.h>
#include <stddef.h>

struct dps310_device {
    void *bus_handle;
    uint8_t i2c_addr;
};

int dps310_init(struct dps310_device *dev, void *bus_handle);
int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_raw);
int dps310_read_temp(struct dps310_device *dev, int32_t *temp_raw);

#endif /* DPS310_H */