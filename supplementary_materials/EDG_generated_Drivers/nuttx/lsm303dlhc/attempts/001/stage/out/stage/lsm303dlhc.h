#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

struct i2c_master_s;

struct lsm303dlhc_dev_s {
    struct i2c_master_s *bus;
    uint8_t accel_addr;
    uint8_t mag_addr;
};

int lsm303dlhc_init(struct lsm303dlhc_dev_s *dev, struct i2c_master_s *bus);
int lsm303dlhc_read_accel(struct lsm303dlhc_dev_s *dev, int16_t *ax, int16_t *ay, int16_t *az);
int lsm303dlhc_read_mag(struct lsm303dlhc_dev_s *dev, int16_t *mx, int16_t *my, int16_t *mz);
int lsm303dlhc_read_temp(struct lsm303dlhc_dev_s *dev, int32_t *t);

#endif /* LSM303DLHC_H */