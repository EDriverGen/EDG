#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "threadx.h"
struct tmp421_device {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int tmp421_init(struct tmp421_device *dev, void *bus_handle);
int tmp421_read_temperature_local(struct tmp421_device *dev, int32_t *temp_local_val);
int tmp421_read_temperature_remote1(struct tmp421_device *dev, int32_t *temp_remote_val);

#endif
