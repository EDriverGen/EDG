#ifndef MHZ19B_H
#define MHZ19B_H

#include <stdint.h>

struct mhz19b_dev {
    int fd;
};

int mhz19b_init(struct mhz19b_dev *dev, const char *bus_name);
int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw);

#endif /* MHZ19B_H */