/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Accelerometer + Magnetometer Driver for XiUOS
 */
#ifndef __LSM303DLHC_REF_H
#define __LSM303DLHC_REF_H

#include <transform.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LSM303DLHC_ACCEL_ADDR        0x19
#define LSM303DLHC_ADDR_ACCEL  LSM303DLHC_ACCEL_ADDR  /* alias */
#define LSM303DLHC_MAG_ADDR          0x1E

/* Accelerometer registers */
#define LSM303_CTRL_REG1_A           0x20
#define LSM303_CTRL_REG4_A           0x23
#define LSM303_OUT_X_L_A             0x28

/* Magnetometer registers */
#define LSM303_CRA_REG_M             0x00
#define LSM303_MR_REG_M              0x02
#define LSM303_OUT_X_H_M             0x03
#define LSM303_IRA_REG_M             0x0A
#define LSM303_IRB_REG_M             0x0B
#define LSM303_IRC_REG_M             0x0C

#define LSM303_IRA_VALUE             0x48
#define LSM303_IRB_VALUE             0x34
#define LSM303_IRC_VALUE             0x33

#define LSM303_ACCEL_ODR_50HZ        0x47
#define LSM303_ACCEL_FS_2G           0x00
#define LSM303_MAG_ODR_15HZ          0x10
#define LSM303_MAG_CONTINUOUS         0x00

struct lsm303dlhc_accel_data { int16_t x, y, z; };
struct lsm303dlhc_mag_data   { int16_t x, y, z; };

struct lsm303dlhc_device
{
  int accel_fd;
  int mag_fd;
};

int lsm303dlhc_init(struct lsm303dlhc_device *dev,
                    const char *i2c_dev_path);
void lsm303dlhc_deinit(struct lsm303dlhc_device *dev);
int lsm303dlhc_probe(struct lsm303dlhc_device *dev);
int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev);
int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev);
int lsm303dlhc_read_accel(struct lsm303dlhc_device *dev,
                          struct lsm303dlhc_accel_data *data);
int lsm303dlhc_read_mag(struct lsm303dlhc_device *dev,
                        struct lsm303dlhc_mag_data *data);

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
