#ifndef BH1750_CMSIS_RTX_REF_H
#define BH1750_CMSIS_RTX_REF_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define BH1750_ADDR_LOW         0x23
#define BH1750_ADDR_HIGH        0x5C
#define BH1750_DEFAULT_ADDR     BH1750_ADDR_LOW
#define BH1750_CONT_H_RES_MODE  0x10
#define BH1750_ONE_H_RES_MODE   0x20

struct bh1750_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
    uint8_t mode;
};

int bh1750_init(struct bh1750_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int bh1750_probe(struct bh1750_device *dev);
int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw);
uint32_t bh1750_raw_to_lux_x100(uint16_t raw);
int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100);

#endif
