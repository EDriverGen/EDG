#ifndef DPS310_H
#define DPS310_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define DPS310_I2C_ADDR 0x77

#include "cmsis_rtx.h"
struct dps310_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int dps310_init(struct dps310_dev *dev, void *bus_handle);
int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw);
int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw);

#endif
