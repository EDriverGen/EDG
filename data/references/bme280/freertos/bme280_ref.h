#ifndef __BME280_REF_H
#define __BME280_REF_H
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define BME280_ADDR_LOW      0x76
#define BME280_ADDR_HIGH     0x77
#define BME280_ADDR_DEFAULT  BME280_ADDR_LOW
#define BME280_CHIP_ID       0x60

struct bme280_cal {
    uint16_t dig_T1; int16_t dig_T2, dig_T3;
    uint16_t dig_P1; int16_t dig_P2,dig_P3,dig_P4,dig_P5,dig_P6,dig_P7,dig_P8,dig_P9;
    uint8_t dig_H1; int16_t dig_H2; uint8_t dig_H3; int16_t dig_H4,dig_H5; int8_t dig_H6;
};

struct bme280_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
    struct bme280_cal cal;
    int32_t t_fine;
};

int bme280_init(struct bme280_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int bme280_probe(struct bme280_device *dev);
int bme280_read_calibration(struct bme280_device *dev);
int bme280_read(struct bme280_device *dev, int32_t *temp_mcelsius, uint32_t *press_pa, uint32_t *hum_mpercent);
#endif
