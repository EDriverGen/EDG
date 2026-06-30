/*
 * SPDX-License-Identifier: MIT
 *
 * TMP105 Temperature Sensor Driver for ThreadX
 */
#ifndef __TMP105_REF_H
#define __TMP105_REF_H

#include <tx_api.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TMP105_ADDR_DEFAULT           0x48
#define TMP105_REG_TEMP               0x00
#define TMP105_REG_CONF               0x01
#define TMP105_REG_TLOW               0x02
#define TMP105_REG_THIGH              0x03

#define TMP105_CONF_SD                (1U << 0)
#define TMP105_CONF_TM                (1U << 1)
#define TMP105_CONF_RES_SHIFT         5
#define TMP105_CONF_RES_MASK          (0x03U << TMP105_CONF_RES_SHIFT)


struct tmp105_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct tmp105_device
{
    void *bus_context;
    const struct tmp105_i2c_ops *ops;
    uint16_t addr;
};

int tmp105_init(struct tmp105_device *dev, void *bus_context, const struct tmp105_i2c_ops *ops, uint16_t addr);
int tmp105_probe(struct tmp105_device *dev);
int tmp105_read_temperature(struct tmp105_device *dev, int32_t *temp_mcelsius);
int tmp105_set_resolution(struct tmp105_device *dev, uint8_t res_bits);

#ifdef __cplusplus
}
#endif

#endif /* __TMP105_REF_H */
