#ifndef DRIVERGEN_RTEMS_LINUX_I2C_DEV_H
#define DRIVERGEN_RTEMS_LINUX_I2C_DEV_H

#include "linux/i2c.h"

#define I2C_RETRIES     0x701
#define I2C_TIMEOUT     0x702
#define I2C_SLAVE       0x703
#define I2C_TENBIT      0x704
#define I2C_FUNCS       0x705
#define I2C_SLAVE_FORCE 0x706
#define I2C_RDWR        0x707
#define I2C_PEC         0x708
#define I2C_SMBUS       0x720

struct i2c_rdwr_ioctl_data {
    struct i2c_msg *msgs;
    uint32_t nmsgs;
};

#define I2C_RDRW_IOCTL_MAX_MSGS 42

#endif
