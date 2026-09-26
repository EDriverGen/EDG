#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>

struct device;

int bh1750_init(const struct device *dev);
int bh1750_sample_fetch(const struct device *dev, int32_t *raw);

#endif /* BH1750_H */
