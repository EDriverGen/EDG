#ifndef MHZ19B_H
#define MHZ19B_H

#include <stdint.h>
#include <stddef.h>

struct mhz19b_dev {
    void *bus_handle;
};

int mhz19b_init(struct mhz19b_dev *dev, void *bus_handle);
int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw);

#endif