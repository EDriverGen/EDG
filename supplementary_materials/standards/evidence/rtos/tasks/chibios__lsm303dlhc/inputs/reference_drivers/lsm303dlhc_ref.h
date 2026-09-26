/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Accelerometer/Magnetometer Driver for ChibiOS
 */
#ifndef __LSM303DLHC_REF_H
#define __LSM303DLHC_REF_H

#include "hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LSM303DLHC_ADDR_ACCEL             0x19
#define LSM303DLHC_ADDR_MAG               0x1E

#define LSM303DLHC_REG_CTRL_REG1_A        0x20
#define LSM303DLHC_REG_CTRL_REG4_A        0x23
#define LSM303DLHC_REG_OUT_X_L_A          0x28
#define LSM303DLHC_REG_CRA_REG_M          0x00
#define LSM303DLHC_REG_CRB_REG_M          0x01
#define LSM303DLHC_REG_MR_REG_M           0x02
#define LSM303DLHC_REG_OUT_X_H_M          0x03

struct lsm303dlhc_device
{
    I2CDriver *bus;     /* ChibiOS I2C driver pointer */
    uint16_t accel_addr;
    uint16_t mag_addr;
};

int lsm303dlhc_init(struct lsm303dlhc_device *dev, I2CDriver *bus, uint16_t accel_addr);
int lsm303dlhc_probe(struct lsm303dlhc_device *dev);
int lsm303dlhc_enable_accel(struct lsm303dlhc_device *dev);
int lsm303dlhc_enable_mag(struct lsm303dlhc_device *dev);
int lsm303dlhc_read_accel(struct lsm303dlhc_device *dev, int16_t *x, int16_t *y, int16_t *z);
int lsm303dlhc_read_mag(struct lsm303dlhc_device *dev, int16_t *x, int16_t *y, int16_t *z);

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
    return lsm303dlhc_read_accel(dev, &out->x, &out->y, &out->z);
}
static inline int lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *out) {
    return lsm303dlhc_read_mag(dev, &out->x, &out->y, &out->z);
}

/* EVAL_START_SHIM */
static inline int lsm303dlhc_start(struct lsm303dlhc_device *dev) {
    int err = lsm303dlhc_enable_accel(dev);
    if (err) return err;
    return lsm303dlhc_enable_mag(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* __LSM303DLHC_REF_H */
