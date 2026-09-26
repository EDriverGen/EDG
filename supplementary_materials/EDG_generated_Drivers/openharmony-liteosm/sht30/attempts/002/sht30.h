#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>
#include "i2c_if.h"

#define SHT30_I2C_ADDR 0x44

#include "openharmony_liteosm.h"
struct sht30_dev {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int sht30_init(struct sht30_dev *dev, DevHandle bus_handle);
int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent);

#endif
