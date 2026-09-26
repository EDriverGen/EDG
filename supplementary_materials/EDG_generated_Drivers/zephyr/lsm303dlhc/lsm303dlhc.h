#ifndef LSM303DLHC_H
#define LSM303DLHC_H

#include <stdint.h>
#include <zephyr/device.h>

#include <zephyr/drivers/i2c.h>
struct device;

/* Accelerometer I2C address (7-bit) */
#define LSM303DLHC_ACCEL_ADDR 0x19

/* Magnetometer I2C address (7-bit) */
#define LSM303DLHC_MAG_ADDR 0x1E

/* Register addresses */
#define LSM303DLHC_CTRL_REG1_A 0x20
#define LSM303DLHC_CTRL_REG4_A 0x23
#define LSM303DLHC_CRA_REG_M 0x00
#define LSM303DLHC_CRB_REG_M 0x01
#define LSM303DLHC_MR_REG_M 0x02
#define LSM303DLHC_OUT_X_L_A 0x28
#define LSM303DLHC_OUT_X_H_M 0x03
#define LSM303DLHC_TEMP_OUT_H_M 0x31

/* Public API functions */
int lsm303dlhc_init(const struct device *dev);
int lsm303dlhc_read_accel(const struct device *dev, int16_t *ax, int16_t *ay, int16_t *az);
int lsm303dlhc_read_mag(const struct device *dev, int16_t *mx, int16_t *my, int16_t *mz);
int lsm303dlhc_read_temp(const struct device *dev, int32_t *t);

#endif /* LSM303DLHC_H */
