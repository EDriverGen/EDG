#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "threadx.h"
struct sht30_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int sht30_init(struct sht30_dev *dev, void *bus_handle);
int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent);

#endif /* SHT30_H */
