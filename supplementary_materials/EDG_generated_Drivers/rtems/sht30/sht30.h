#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>

struct sht30_device {
    int fd;
    uint8_t i2c_addr;
};

int sht30_init(struct sht30_device *dev, void *bus_handle);
int sht30_read_measurement(struct sht30_device *dev, int32_t *temp_milliC, int32_t *humidity_milliPct);

#endif /* SHT30_H */