#ifndef TMP105_H
#define TMP105_H

#include <stdint.h>

struct tmp105_dev {
    int fd;
    uint8_t addr;
};

int tmp105_init(struct tmp105_dev *dev, void *bus_handle);
int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw);

#endif