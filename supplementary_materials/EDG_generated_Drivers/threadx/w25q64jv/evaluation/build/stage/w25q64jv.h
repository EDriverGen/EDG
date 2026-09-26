#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *bus_handle;
    uint8_t i2c_addr;
} w25q64jv_dev_t;

int w25q64jv_init(w25q64jv_dev_t *dev, void *bus_handle);
int w25q64jv_read(w25q64jv_dev_t *dev, uint32_t addr, uint8_t *buf, uint32_t len);
int w25q64jv_write(w25q64jv_dev_t *dev, uint32_t addr, const uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* W25Q64JV_H */