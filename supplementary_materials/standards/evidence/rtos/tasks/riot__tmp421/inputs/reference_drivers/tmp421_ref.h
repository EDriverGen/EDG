/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * TMP421 Remote Temperature Sensor Driver for RIOT OS
 */
#ifndef __TMP421_REF_H
#define __TMP421_REF_H

#include "periph/i2c.h"
#include "ztimer.h"
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TMP421_ADDR_DEFAULT           0x2A

#define TMP421_REG_LOCAL_TEMP_H       0x00
#define TMP421_REG_LOCAL_TEMP_L       0x10
#define TMP421_REG_REMOTE_TEMP_H      0x01
#define TMP421_REG_REMOTE_TEMP_L      0x11
#define TMP421_REG_STATUS             0x08
#define TMP421_REG_CONFIG1            0x09
#define TMP421_REG_MFG_ID             0xFE
#define TMP421_REG_DEV_ID             0xFF

#define TMP421_MFG_ID_EXPECTED        0x55

struct tmp421_device
{
    i2c_t bus;          /* RIOT I2C device index */
    uint16_t addr;
};

int tmp421_init(struct tmp421_device *dev, i2c_t bus, uint16_t addr);
int tmp421_probe(struct tmp421_device *dev);
int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius);
int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius);

#ifdef __cplusplus
}
#endif

#endif /* __TMP421_REF_H */
