/*
 * SPDX-License-Identifier: MIT
 *
 * TMP421 Remote Temperature Sensor Driver for ThreadX
 */
#ifndef __TMP421_REF_H
#define __TMP421_REF_H

#include <tx_api.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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
#define TMP421_DEV_ID_EXPECTED        0x21


struct tmp421_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct tmp421_device
{
    void *bus_context;
    const struct tmp421_i2c_ops *ops;
    uint16_t addr;
};

int tmp421_init(struct tmp421_device *dev, void *bus_context, const struct tmp421_i2c_ops *ops, uint16_t addr);
int tmp421_probe(struct tmp421_device *dev);
int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius);
int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius);

#ifdef __cplusplus
}
#endif

#endif /* __TMP421_REF_H */
