#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>

struct sht30_dev {
    void *bus_handle;
    uint8_t i2c_addr;
};

int sht30_init(struct sht30_dev *dev, void *bus_handle);
int sht30_read_temp_humidity(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent);

#endif