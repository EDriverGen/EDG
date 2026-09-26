#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include <stdint.h>
#include <rtdevice.h>

#define LSM303DLHC_ACCEL_ADDR 0x19
#define LSM303DLHC_MAG_ADDR   0x1E

typedef struct {
    struct rt_i2c_bus_device *bus;
} lsm303dlhc_device_t;

int lsm303dlhc_init(lsm303dlhc_device_t *dev, struct rt_i2c_bus_device *bus);
int lsm303dlhc_read_accel_x(lsm303dlhc_device_t *dev, int32_t *ax);
int lsm303dlhc_read_accel_y(lsm303dlhc_device_t *dev, int32_t *ay);
int lsm303dlhc_read_accel_z(lsm303dlhc_device_t *dev, int32_t *az);
int lsm303dlhc_read_mag_x(lsm303dlhc_device_t *dev, int32_t *mx);
int lsm303dlhc_read_mag_y(lsm303dlhc_device_t *dev, int32_t *my);
int lsm303dlhc_read_mag_z(lsm303dlhc_device_t *dev, int32_t *mz);
int lsm303dlhc_read_temperature(lsm303dlhc_device_t *dev, int32_t *temp);

#endif
