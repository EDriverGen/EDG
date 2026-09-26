#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>

#include <hal/hal_i2c.h>
struct sht30_dev {
    uint8_t i2c_num;
    uint8_t i2c_addr;
};

int sht30_init(struct sht30_dev *dev, void *bus_handle);
int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_val, int32_t *hum_val);

#endif /* SHT30_H */
