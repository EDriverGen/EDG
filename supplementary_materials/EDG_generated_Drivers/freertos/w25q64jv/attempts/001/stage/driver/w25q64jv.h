#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>

struct w25q64jv_dev {
    void *bus_handle;
    uint8_t cs_pin;
};

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle);
int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len);
int w25q64jv_write_page(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len);

#endif /* W25Q64JV_H */