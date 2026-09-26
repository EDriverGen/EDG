/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add LSM303DLHC driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_LSM303DLHC_H_
#define DRIVERS_INCLUDE_LSM303DLHC_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * LSM303DLHC has two separate I2C slave addresses for its accelerometer and magnetometer.
 * The addresses are fixed and cannot be configured by external pins.
 */
#define LSM303DLHC_ACCEL_ADDR        0x19   /* Accelerometer I2C address */
#define LSM303DLHC_MAG_ADDR          0x1E   /* Magnetometer I2C address */

#define LSM303DLHC_DEFAULT_BUS_NAME  "i2c1"

/* ---- Accelerometer registers ---- */
#define LSM303DLHC_CTRL_REG1_A       0x20
#define LSM303DLHC_CTRL_REG4_A       0x23
#define LSM303DLHC_STATUS_REG_A      0x27
#define LSM303DLHC_OUT_X_L_A         0x28
#define LSM303DLHC_OUT_X_H_A         0x29
#define LSM303DLHC_OUT_Y_L_A         0x2A
#define LSM303DLHC_OUT_Y_H_A         0x2B
#define LSM303DLHC_OUT_Z_L_A         0x2C
#define LSM303DLHC_OUT_Z_H_A         0x2D

/* CTRL_REG1_A bit definitions */
#define LSM303DLHC_ODR_1HZ           0x10
#define LSM303DLHC_ODR_10HZ          0x20
#define LSM303DLHC_ODR_25HZ          0x30
#define LSM303DLHC_ODR_50HZ          0x40
#define LSM303DLHC_ODR_100HZ         0x50
#define LSM303DLHC_AXES_ENABLE       0x07   /* Enable all X/Y/Z axes */

/* CTRL_REG4_A bit definitions */
#define LSM303DLHC_FS_2G             0x00
#define LSM303DLHC_FS_4G             0x10
#define LSM303DLHC_FS_8G             0x20
#define LSM303DLHC_FS_16G            0x30
#define LSM303DLHC_HR_BIT            0x08   /* High-resolution mode */

/* ---- Magnetometer registers ---- */
#define LSM303DLHC_CRA_REG_M         0x00   /* Output data rate */
#define LSM303DLHC_CRB_REG_M         0x01   /* Gain configuration */
#define LSM303DLHC_MR_REG_M          0x02   /* Mode register */
#define LSM303DLHC_OUT_X_H_M         0x03
#define LSM303DLHC_OUT_X_L_M         0x04
#define LSM303DLHC_OUT_Z_H_M         0x05
#define LSM303DLHC_OUT_Z_L_M         0x06
#define LSM303DLHC_OUT_Y_H_M         0x07
#define LSM303DLHC_OUT_Y_L_M         0x08
#define LSM303DLHC_SR_REG_M          0x09   /* Status register */
#define LSM303DLHC_IRA_REG_M         0x0A   /* Identification register A: 0x48 */
#define LSM303DLHC_IRB_REG_M         0x0B   /* Identification register B: 0x34 */
#define LSM303DLHC_IRC_REG_M         0x0C   /* Identification register C: 0x33 */

/* Magnetometer modes */
#define LSM303DLHC_MAG_CONTINUOUS    0x00
#define LSM303DLHC_MAG_SINGLE        0x01
#define LSM303DLHC_MAG_SLEEP         0x03

/* Magnetometer gains */
#define LSM303DLHC_MAG_GAIN_1_3      0x20   /* ±1.3 gauss, 1100 LSB/gauss */
#define LSM303DLHC_MAG_GAIN_1_9      0x40
#define LSM303DLHC_MAG_GAIN_4_0      0xC0

/* Magnetometer data rates */
#define LSM303DLHC_MAG_ODR_15HZ      0x10
#define LSM303DLHC_MAG_ODR_30HZ      0x14
#define LSM303DLHC_MAG_ODR_75HZ      0x18

/* Expected identification-register values */
#define LSM303DLHC_IRA_VALUE         0x48
#define LSM303DLHC_IRB_VALUE         0x34
#define LSM303DLHC_IRC_VALUE         0x33

/*
 * Three-axis data structure.
 */
struct lsm303dlhc_xyz
{
    rt_int16_t x;
    rt_int16_t y;
    rt_int16_t z;
};

/*
 * LSM303DLHC driver device object.
 */
struct lsm303dlhc_device
{
    struct rt_i2c_bus_device *bus;
    const char *bus_name;
};

/*
 * Initialize a device object.
 */
rt_err_t lsm303dlhc_init(struct lsm303dlhc_device *dev,
                          const char               *bus_name);

/*
 * Probe the magnetometer by reading its identification registers.
 */
rt_err_t lsm303dlhc_probe(struct lsm303dlhc_device *dev);

/*
 * Configure and start the accelerometer; defaults: 50Hz, ±2g, high resolution.
 */
rt_err_t lsm303dlhc_accel_start(struct lsm303dlhc_device *dev);

/*
 * Read raw accelerometer values.
 */
rt_err_t lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev,
                                    struct lsm303dlhc_xyz    *accel);

/*
 * Configure and start the magnetometer; defaults: continuous mode, 15Hz, ±1.3 gauss.
 */
rt_err_t lsm303dlhc_mag_start(struct lsm303dlhc_device *dev);

/*
 * Read raw magnetometer values.
 */
rt_err_t lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev,
                                  struct lsm303dlhc_xyz    *mag);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_LSM303DLHC_H_ */
