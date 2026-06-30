/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Accelerometer + Magnetometer Driver for NuttX
 */
#ifndef __LSM303DLHC_REF_H
#define __LSM303DLHC_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LSM303DLHC_ACCEL_ADDR        0x19
#define LSM303DLHC_ADDR_ACCEL  LSM303DLHC_ACCEL_ADDR  /* alias */
#define LSM303DLHC_MAG_ADDR          0x1E
#define LSM303DLHC_I2C_FREQ          100000

/* Accelerometer registers */
#define LSM303_CTRL_REG1_A           0x20
#define LSM303_CTRL_REG4_A           0x23
#define LSM303_OUT_X_L_A             0x28
#define LSM303_OUT_X_H_A             0x29
#define LSM303_OUT_Y_L_A             0x2A
#define LSM303_OUT_Y_H_A             0x2B
#define LSM303_OUT_Z_L_A             0x2C
#define LSM303_OUT_Z_H_A             0x2D

/* Magnetometer registers */
#define LSM303_CRA_REG_M             0x00
#define LSM303_CRB_REG_M             0x01
#define LSM303_MR_REG_M              0x02
#define LSM303_OUT_X_H_M             0x03
#define LSM303_OUT_X_L_M             0x04
#define LSM303_OUT_Z_H_M             0x05
#define LSM303_OUT_Z_L_M             0x06
#define LSM303_OUT_Y_H_M             0x07
#define LSM303_OUT_Y_L_M             0x08
#define LSM303_IRA_REG_M             0x0A
#define LSM303_IRB_REG_M             0x0B
#define LSM303_IRC_REG_M             0x0C

/* Identification register expected values */
#define LSM303_IRA_VALUE             0x48
#define LSM303_IRB_VALUE             0x34
#define LSM303_IRC_VALUE             0x33

/* CTRL_REG1_A: ODR[3:0] | LPen | Zen | Yen | Xen */
#define LSM303_ACCEL_ODR_50HZ       0x47  /* 50 Hz, all axes, normal mode */
/* CTRL_REG4_A: BDU | BLE | FS[1:0] | HR | 0 | 0 | SIM */
#define LSM303_ACCEL_FS_2G          0x00

/* CRA_REG_M: TEMP_EN | 0 | 0 | DO[2:0] | 0 | 0 */
#define LSM303_MAG_ODR_15HZ         0x10
/* MR_REG_M: 0 | 0 | 0 | 0 | 0 | 0 | MD[1:0] */
#define LSM303_MAG_CONTINUOUS        0x00

struct lsm303dlhc_accel_data
{
  int16_t x;
  int16_t y;
  int16_t z;
};

struct lsm303dlhc_mag_data
{
  int16_t x;
  int16_t y;
  int16_t z;
};

struct lsm303dlhc_device
{
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s accel_config;
  struct i2c_config_s mag_config;
};

int lsm303dlhc_init(FAR struct lsm303dlhc_device *dev,
                    FAR struct i2c_master_s *i2c);
int lsm303dlhc_probe(FAR struct lsm303dlhc_device *dev);

int lsm303dlhc_accel_start(FAR struct lsm303dlhc_device *dev);
int lsm303dlhc_mag_start(FAR struct lsm303dlhc_device *dev);

int lsm303dlhc_read_accel(FAR struct lsm303dlhc_device *dev,
                          FAR struct lsm303dlhc_accel_data *data);
int lsm303dlhc_read_mag(FAR struct lsm303dlhc_device *dev,
                        FAR struct lsm303dlhc_mag_data *data);

/* Three-axis data struct for raw read API */
struct lsm303dlhc_xyz {
    int16_t x;
    int16_t y;
    int16_t z;
};

int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev);

int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev);

#ifdef __cplusplus
#endif

/* EVAL_COMPAT_SHIM */
static inline int lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *out) {
    struct lsm303dlhc_accel_data d;
    int err = lsm303dlhc_read_accel(dev, &d);
    if (out) { out->x = d.x; out->y = d.y; out->z = d.z; }
    return err;
}
static inline int lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *out) {
    struct lsm303dlhc_mag_data d;
    int err = lsm303dlhc_read_mag(dev, &d);
    if (out) { out->x = d.x; out->y = d.y; out->z = d.z; }
    return err;
}

/* EVAL_START_SHIM */
static inline int lsm303dlhc_start(struct lsm303dlhc_device *dev) {
    int err = lsm303dlhc_accel_start(dev);
    if (err) return err;
    return lsm303dlhc_mag_start(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* __LSM303DLHC_REF_H */
