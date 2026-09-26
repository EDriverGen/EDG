#ifndef MHZ19B_H
#define MHZ19B_H

#include <stdint.h>

struct mhz19b_device {
    int fd;
};

int mhz19b_init(struct mhz19b_device *dev, const char *bus_handle);
int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw);

#endif