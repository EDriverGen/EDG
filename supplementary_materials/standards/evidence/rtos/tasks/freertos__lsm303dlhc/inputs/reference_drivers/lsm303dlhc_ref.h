#ifndef __LSM303DLHC_REF_H
#define __LSM303DLHC_REF_H

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define LSM303DLHC_ADDR_ACCEL       0x19
#define LSM303DLHC_ADDR_MAG         0x1E
#define LSM303DLHC_REG_CTRL_REG1_A  0x20
#define LSM303DLHC_REG_OUT_X_L_A    0x28
#define LSM303DLHC_REG_CRA_REG_M    0x00
#define LSM303DLHC_REG_CRB_REG_M    0x01
#define LSM303DLHC_REG_MR_REG_M     0x02
#define LSM303DLHC_REG_OUT_X_H_M    0x03
#define LSM303DLHC_REG_IRA_REG_M    0x0A
#define LSM303DLHC_REG_IRB_REG_M    0x0B
#define LSM303DLHC_REG_IRC_REG_M    0x0C
#define LSM303DLHC_IRA_VALUE        0x48
#define LSM303DLHC_IRB_VALUE        0x34
#define LSM303DLHC_IRC_VALUE        0x33

/* Three-axis data struct matching RT-Thread API */
struct lsm303dlhc_xyz {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct lsm303dlhc_device {
    I2C_HandleTypeDef *bus;
    uint16_t accel_addr;
    uint16_t mag_addr;
};

int lsm303dlhc_init(struct lsm303dlhc_device *dev, I2C_HandleTypeDef *bus, uint16_t accel_addr);
int lsm303dlhc_probe(struct lsm303dlhc_device *dev);
int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev);
int lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *accel);
int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev);
int lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *mag);
int lsm303dlhc_enable_accel(struct lsm303dlhc_device *dev);
int lsm303dlhc_enable_mag(struct lsm303dlhc_device *dev);
int lsm303dlhc_read_accel(struct lsm303dlhc_device *dev, int16_t *x, int16_t *y, int16_t *z);
int lsm303dlhc_read_mag(struct lsm303dlhc_device *dev, int16_t *x, int16_t *y, int16_t *z);

/* EVAL_START_SHIM */

/* Compatibility shim: adapter calls lsm303dlhc_start() */
static inline int lsm303dlhc_start(struct lsm303dlhc_device *dev) {
    int err = lsm303dlhc_accel_start(dev);
    if (err) return err;
    return lsm303dlhc_mag_start(dev);
}


#endif
