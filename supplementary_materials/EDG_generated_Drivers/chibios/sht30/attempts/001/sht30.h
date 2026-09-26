#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>
#include "hal_i2c.h"

struct sht30_dev {
    I2CDriver *bus;
    uint8_t addr;
};

void sht30_init(struct sht30_dev *dev, void *bus_handle);
void sht30_read_temp_humidity(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent);

#endif /* SHT30_H */