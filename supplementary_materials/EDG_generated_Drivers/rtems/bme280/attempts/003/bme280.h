#ifndef BME280_H
#define BME280_H

#include <stdint.h>

struct bme280_dev {
    int fd;
    uint8_t addr;
};

int bme280_init(struct bme280_dev *dev, const char *bus_handle);
int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw);

#endif