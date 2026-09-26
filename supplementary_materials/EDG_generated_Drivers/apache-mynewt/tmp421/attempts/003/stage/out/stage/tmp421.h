#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>

#include <hal/hal_i2c.h>
struct tmp421_dev {
    uint8_t i2c_num;
    uint8_t addr;
};

int tmp421_init(struct tmp421_dev *dev, void *bus_handle);
int tmp421_read_temp_local(struct tmp421_dev *dev, int32_t *temp);
int tmp421_read_temp_remote(struct tmp421_dev *dev, int32_t *temp);

#endif
