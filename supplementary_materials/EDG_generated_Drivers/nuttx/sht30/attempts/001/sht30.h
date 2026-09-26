#ifndef SHT30_H
#define SHT30_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

struct i2c_master_s;

struct sht30_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int sht30_init(struct sht30_dev_s *dev, struct i2c_master_s *bus);
int sht30_read_measurement(struct sht30_dev_s *dev, int32_t *temp_milliC, int32_t *hum_milliPct);

#endif /* SHT30_H */