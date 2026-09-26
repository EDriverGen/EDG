/*
 * SPDX-License-Identifier: MIT
 *
 * EMC1413 Temperature Sensor Driver for ThreadX
 */
#ifndef __EMC1413_REF_H
#define __EMC1413_REF_H

#include <tx_api.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EMC1413_ADDR_DEFAULT           0x4C

#define EMC1413_REG_INTERNAL_TEMP      0x00
#define EMC1413_REG_INTERNAL_TEMP_L    0x29
#define EMC1413_REG_EXT1_TEMP_H        0x01
#define EMC1413_REG_EXT1_TEMP_L        0x10
#define EMC1413_REG_EXT2_TEMP_H        0x23
#define EMC1413_REG_EXT2_TEMP_L        0x24
#define EMC1413_REG_STATUS             0x02
#define EMC1413_REG_CONFIG             0x03
#define EMC1413_REG_MFG_ID             0xFE
#define EMC1413_REG_PRODUCT_ID         0xFD
#define EMC1413_REG_REVISION           0xFF

#define EMC1413_MFG_ID_EXPECTED        0x5D
#define EMC1413_PRODUCT_ID_EXPECTED    0x21


struct emc1413_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct emc1413_device
{
    void *bus_context;
    const struct emc1413_i2c_ops *ops;
    uint16_t addr;
};

/* Channel enum matching RT-Thread API */
enum emc1413_channel {
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};

int emc1413_init(struct emc1413_device *dev, void *bus_context, const struct emc1413_i2c_ops *ops, uint16_t addr);
int emc1413_probe(struct emc1413_device *dev);

int emc1413_read_internal_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);
int emc1413_read_external1_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);
int emc1413_read_external2_temp(struct emc1413_device *dev, int32_t *temp_mcelsius);
int emc1413_read_temperature(struct emc1413_device *dev, enum emc1413_channel channel,
                             int32_t *temp_mcelsius);

#ifdef __cplusplus
}
#endif

#endif /* __EMC1413_REF_H */
