#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include <stdint.h>

#include <hal/hal_i2c.h>
#ifdef __cplusplus
extern "C" {
#endif

struct lsm303dlhc_dev {
    uint8_t i2c_num;
    uint8_t accel_addr;
    uint8_t mag_addr;
};

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle);
int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az);
int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz);
int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t);

#ifdef __cplusplus
}
#endif

#endif /* LSM303DLHC_H */
