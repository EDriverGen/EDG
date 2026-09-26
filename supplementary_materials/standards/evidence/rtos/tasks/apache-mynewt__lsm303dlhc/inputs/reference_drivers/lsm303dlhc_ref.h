#ifndef LSM303DLHC_APACHE_MYNEWT_REF_H
#define LSM303DLHC_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_i2c.h"
#include <stdint.h>

#define LSM303DLHC_ACCEL_ADDR        0x19
#define LSM303DLHC_MAG_ADDR          0x1E

#define LSM303DLHC_CTRL_REG1_A       0x20
#define LSM303DLHC_CTRL_REG4_A       0x23
#define LSM303DLHC_OUT_X_L_A         0x28

#define LSM303DLHC_ODR_50HZ          0x40
#define LSM303DLHC_AXES_ENABLE       0x07
#define LSM303DLHC_FS_2G             0x00
#define LSM303DLHC_HR_BIT            0x08

#define LSM303DLHC_CRA_REG_M         0x00
#define LSM303DLHC_CRB_REG_M         0x01
#define LSM303DLHC_MR_REG_M          0x02
#define LSM303DLHC_OUT_X_H_M         0x03
#define LSM303DLHC_IRA_REG_M         0x0A
#define LSM303DLHC_IRB_REG_M         0x0B
#define LSM303DLHC_IRC_REG_M         0x0C

#define LSM303DLHC_MAG_CONTINUOUS    0x00
#define LSM303DLHC_MAG_GAIN_1_3      0x20
#define LSM303DLHC_MAG_ODR_15HZ      0x10

#define LSM303DLHC_IRA_VALUE         0x48
#define LSM303DLHC_IRB_VALUE         0x34
#define LSM303DLHC_IRC_VALUE         0x33

struct lsm303dlhc_xyz {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct lsm303dlhc_device {
    uint8_t i2c_num;
};

int lsm303dlhc_init(struct lsm303dlhc_device *dev, uint8_t i2c_num);
int lsm303dlhc_probe(struct lsm303dlhc_device *dev);
int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev);
int lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *accel);
int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev);
int lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *mag);

#endif
