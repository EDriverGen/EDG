#ifndef TMP421_H
#define TMP421_H

#include <stdint.h>

int tmp421_init(void);
int tmp421_read_temperature_local(void *dev, int32_t *val);
int tmp421_read_temperature_remote1(void *dev, int32_t *val);

#endif /* TMP421_H */