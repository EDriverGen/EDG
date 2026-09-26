#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>
#include "stm32f1xx_hal_i2c.h"

typedef struct {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
} lm75a_device_t;

int lm75a_init(lm75a_device_t *dev, void *bus_handle);
int lm75a_read_temperature(lm75a_device_t *dev, int16_t *raw);

#endif /* LM75A_H */