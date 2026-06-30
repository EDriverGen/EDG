/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * BH1750 Light Sensor Driver for RIOT OS
 */
#ifndef __BH1750_REF_H
#define __BH1750_REF_H

#include "periph/i2c.h"
#include "ztimer.h"
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BH1750_ADDR_LOW              0x23
#define BH1750_ADDR_HIGH             0x5C
#define BH1750_DEFAULT_ADDR          BH1750_ADDR_LOW

#define BH1750_CONT_H_RES_MODE      0x10
#define BH1750_CONT_H_RES_MODE2     0x11
#define BH1750_CONT_L_RES_MODE      0x13
#define BH1750_ONE_H_RES_MODE       0x20
#define BH1750_ONE_H_RES_MODE2      0x21
#define BH1750_ONE_L_RES_MODE       0x23

struct bh1750_device
{
    i2c_t bus;          /* RIOT I2C device index */
    uint16_t addr;
    uint8_t mode;
};

int bh1750_init(struct bh1750_device *dev, i2c_t bus, uint16_t addr);
int bh1750_set_mode(struct bh1750_device *dev, uint8_t mode);
int bh1750_probe(struct bh1750_device *dev);
int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw);
uint32_t bh1750_raw_to_lux_x100(uint16_t raw);
int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100);

#ifdef __cplusplus
}
#endif

#endif /* __BH1750_REF_H */
