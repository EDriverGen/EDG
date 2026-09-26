#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include <stdint.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct rt_i2c_bus_device *bus;
    uint8_t accel_addr;
    uint8_t mag_addr;
    int16_t accel_raw[3];
    int16_t mag_raw[3];
    int16_t temp_raw;
} lsm303dlhc_device_t;

int lsm303dlhc_init(lsm303dlhc_device_t *dev, struct rt_i2c_bus_device *bus);
int lsm303dlhc_read_accel_x(lsm303dlhc_device_t *dev, int32_t *ax);
int lsm303dlhc_read_accel_y(lsm303dlhc_device_t *dev, int32_t *ay);
int lsm303dlhc_read_accel_z(lsm303dlhc_device_t *dev, int32_t *az);
int lsm303dlhc_read_mag_x(lsm303dlhc_device_t *dev, int32_t *mx);
int lsm303dlhc_read_mag_y(lsm303dlhc_device_t *dev, int32_t *my);
int lsm303dlhc_read_mag_z(lsm303dlhc_device_t *dev, int32_t *mz);
int lsm303dlhc_read_temperature(lsm303dlhc_device_t *dev, int32_t *temp);

#ifdef __cplusplus
}
#endif

#endif /* LSM303DLHC_H */