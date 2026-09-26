#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "cmsis_rtx.h"
struct adxl345_dev {
    SPI_HandleTypeDef *spi;
};

int adxl345_init(struct adxl345_dev *dev, void *bus_handle);
int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az);

#endif
