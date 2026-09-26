#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

struct bh1750_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int bh1750_init(struct bh1750_dev *dev, void *bus_handle);
int bh1750_read(struct bh1750_dev *dev, int32_t *raw);

#endif
