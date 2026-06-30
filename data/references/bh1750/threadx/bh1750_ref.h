/*
 * SPDX-License-Identifier: MIT
 *
 * BH1750 Light Sensor Driver for ThreadX
 */
#ifndef __BH1750_REF_H
#define __BH1750_REF_H

#include <tx_api.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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


struct bh1750_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct bh1750_device
{
    void *bus_context;
    const struct bh1750_i2c_ops *ops;
    uint16_t addr;
    uint8_t mode;
};

int bh1750_init(struct bh1750_device *dev, void *bus_context, const struct bh1750_i2c_ops *ops, uint16_t addr);
int bh1750_set_mode(struct bh1750_device *dev, uint8_t mode);
int bh1750_probe(struct bh1750_device *dev);
int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw);
uint32_t bh1750_raw_to_lux_x100(uint16_t raw);
int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100);

#ifdef __cplusplus
}
#endif

#endif /* __BH1750_REF_H */
