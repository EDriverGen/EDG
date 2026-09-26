#ifndef EMC1413_H
#define EMC1413_H

#include <stdint.h>
#include <nuttx/i2c/i2c_master.h>

#define EMC1413_I2C_ADDR 0x4C

struct i2c_master_s;

struct emc1413_dev {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int emc1413_init(struct emc1413_dev *dev, struct i2c_master_s *bus);
int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp);
int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp);
int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp);

#endif /* EMC1413_H */