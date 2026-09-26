#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define EMC1413_I2C_ADDR 0x4C

#include "cmsis_rtx.h"
struct emc1413_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int emc1413_init(struct emc1413_dev *dev, void *bus_handle);
int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local);
int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1);
int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2);

#endif /* EMC1413_H */
