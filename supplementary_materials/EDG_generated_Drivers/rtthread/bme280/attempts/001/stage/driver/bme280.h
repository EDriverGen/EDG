#ifndef BME280_H
#define BME280_H

#include <stdint.h>

#include <drivers/dev_i2c.h>
struct rt_i2c_bus_device;

struct bme280_dev {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
    int32_t t_fine;
};

int bme280_init(struct bme280_dev *dev, struct rt_i2c_bus_device *bus);
int bme280_read_measurements(struct bme280_dev *dev, int32_t *temp, int32_t *pressure, int32_t *humidity);

#endif
