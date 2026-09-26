#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>
#include "bus_i2c.h"

#define SHT30_I2C_ADDR 0x44

struct I2cBus;

struct sht30_device {
    struct I2cBus *bus;
    int fd;
    uint8_t i2c_addr;
};

int sht30_init(struct sht30_device *dev, struct I2cBus *bus_handle);
int sht30_read_temp_humidity(struct sht30_device *dev, int32_t *temp_milliC, int32_t *humidity_milliPct);

#endif