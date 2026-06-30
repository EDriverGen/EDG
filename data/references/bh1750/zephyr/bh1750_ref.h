/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BH1750 Light Sensor Driver for Zephyr
 */
#ifndef __BH1750_REF_H
#define __BH1750_REF_H

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
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
  const struct device *bus;
  uint16_t addr;
  uint8_t mode;
};

int bh1750_init(struct bh1750_device *dev,
                const struct device *bus,
                uint16_t addr);
int bh1750_set_mode(struct bh1750_device *dev, uint8_t mode);
int bh1750_probe(struct bh1750_device *dev);
int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw);
uint32_t bh1750_raw_to_lux_x100(uint16_t raw);
int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100);

#ifdef __cplusplus
}
#endif

#endif /* __BH1750_REF_H */
