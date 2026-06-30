/* OpenHarmony HDF `i2c_dev.h` sandbox stub for DriverGen.
 *
 * Real upstream:
 *   data/rtos/openharmony-liteosm-project/drivers_hdf_core/adapter/khdf/liteos/platform/include/i2c_dev.h
 *
 * Why this stub exists:
 *   Some generated OpenHarmony drivers include this header for
 *   `struct i2c_msg` (POSIX-style I2C user message), `enum I2cIoctlCmd`
 *   and `enum I2cMsgFlag`.  The upstream is self-contained (~80 LOC),
 *   so we mirror it byte-for-byte except for the include dependency on
 *   `hdf_base.h`, which is already provided by the local stub tree
 *   (forwards to `openharmony_liteosm.h` which carries `uint*_t`).
 *
 *   1. Real upstream file located in `data/rtos/openharmony-liteosm-project/drivers_hdf_core/adapter/khdf/liteos/platform/include/i2c_dev.h` (Apache-2.0).
 *   2. Header is user-facing (#define guard `I2C_USER_H`).
 *   3. All symbols mirrored verbatim: enum values, struct fields, typedef
 *      aliases (`I2cMsgUser`, `I2cIoctlWrap`), public function declarations
 *      (`I2cAddVfsById`, `I2cRemoveVfsById`).
 */
#ifndef I2C_USER_H
#define I2C_USER_H

#include "hdf_base.h"

enum I2cIoctlCmd {
    IOCTL_RETRIES      = 0x0701,
    IOCTL_TIMEOUT      = 0x0702,
    IOCTL_CLIENT       = 0x0703,
    IOCTL_CLIENT_FORCE = 0x0706,
    IOCTL_TENBIT       = 0x0704,
    IOCTL_FUNCS        = 0x0705,
    IOCTL_RDWR         = 0x0707,
    IOCTL_PEC          = 0x0708,
    IOCTL_SMBUS        = 0x0720,
    IOCTL_16BIT_REG    = 0x0709,
    IOCTL_16BIT_DATA   = 0x070a,
};

enum I2cMsgFlag {
    I2C_M_RD           = 0x0001,
    I2C_M_TEN          = 0x0010,
    I2C_M_RECV_LEN     = 0x0400,
    I2C_M_NO_RD_ACK    = 0x0800,
    I2C_M_IGNORE_NAK   = 0x1000,
    I2C_M_REV_DIR_ADDR = 0x2000,
    I2C_M_NOSTART      = 0x4000,
    I2C_M_STOP         = 0x8000,
#ifdef __LITEOS__
    I2C_M_16BIT_DATA   = 0x0008,
    I2C_M_16BIT_REG    = 0x0002,
#endif
};

typedef struct i2c_msg {
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t *buf;
} I2cMsgUser;

typedef struct i2c_rdwr_ioctl_data {
    struct i2c_msg *msgs;
    unsigned int nmsgs;
} I2cIoctlWrap;

int32_t I2cAddVfsById(int16_t id);
void    I2cRemoveVfsById(int16_t id);

#endif /* I2C_USER_H */
