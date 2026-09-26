#ifndef DPS310_H
#define DPS310_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

#define DPS310_I2C_ADDR 0x77

struct i2c_master_s;

struct dps310_dev {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int dps310_init(struct dps310_dev *dev, struct i2c_master_s *bus);
int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw);
int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw);

#endif /* DPS310_H */