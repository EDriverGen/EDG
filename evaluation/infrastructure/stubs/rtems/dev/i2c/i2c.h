#ifndef DRIVERGEN_RTEMS_DEV_I2C_I2C_H
#define DRIVERGEN_RTEMS_DEV_I2C_I2C_H

#include "linux/i2c.h"
#include "linux/i2c-dev.h"
#include "rtems.h"
#include <sys/ioctl.h>

typedef struct i2c_msg i2c_msg;
typedef struct i2c_rdwr_ioctl_data i2c_rdwr_ioctl_data;

#define I2C_BUS_OBTAIN 0x800
#define I2C_BUS_RELEASE 0x801
#define I2C_BUS_SET_CLOCK 0x803
#define I2C_BUS_CLOCK_DEFAULT 100000

#endif
