#ifndef TMP105_H
#define TMP105_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "cmsis_rtx.h"
struct tmp105_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int tmp105_init(struct tmp105_dev *dev, void *bus_handle);
int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw);

#endif
