#ifndef BME280_H
#define BME280_H

#include <stdint.h>

struct i2c_master_s;

struct bme280_dev {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int bme280_init(struct bme280_dev *dev, struct i2c_master_s *bus);
int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw);

#endif