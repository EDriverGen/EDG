#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bh1750_dev_t {
    void *bus_handle;
    uint8_t i2c_addr;
} bh1750_dev_t;

int bh1750_init(bh1750_dev_t *dev, void *bus_handle);
int bh1750_read_illuminance(bh1750_dev_t *dev, int32_t *raw);
int bh1750_set_mtreg(bh1750_dev_t *dev, uint8_t mtreg);

#ifdef __cplusplus
}
#endif

#endif /* BH1750_H */