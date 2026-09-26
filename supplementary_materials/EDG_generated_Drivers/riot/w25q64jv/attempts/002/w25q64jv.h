#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>

#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_SECTOR_SIZE 4096
#define W25Q64JV_MEMORY_SIZE 8388608

#include "riot.h"
typedef struct {
    spi_t bus;
    spi_cs_t cs;
} w25q64jv_t;

int w25q64jv_init(w25q64jv_t *dev, spi_t bus, spi_cs_t cs);
int w25q64jv_read(w25q64jv_t *dev, uint32_t addr, uint8_t *buf, size_t len);
int w25q64jv_write(w25q64jv_t *dev, uint32_t addr, const uint8_t *buf, size_t len);

#endif /* W25Q64JV_H */
