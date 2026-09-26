#ifndef LM75A_H
#define LM75A_H

#include <stdint.h>

struct device;

int lm75a_init(const struct device *dev);
int lm75a_read_temp(const struct device *dev, int32_t *raw);

#endif /* LM75A_H */
