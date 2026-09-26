#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "i2c_if.h"

#include "openharmony_liteosm.h"
struct bme280_dev {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int32_t bme280_init(struct bme280_dev *dev, DevHandle bus_handle);
int32_t bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw);

#endif /* BME280_H */
