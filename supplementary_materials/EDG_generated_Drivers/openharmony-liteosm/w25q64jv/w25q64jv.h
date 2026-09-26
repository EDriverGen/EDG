#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>
#include "spi_if.h"

#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_MEMORY_SIZE 8388608

struct w25q64jv_dev {
    DevHandle spi_handle;
};

int w25q64jv_init(struct w25q64jv_dev *dev, DevHandle bus_handle);
int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, uint32_t len);
int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, uint32_t len);

#endif