#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>

#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_MEMORY_SIZE 8388608

#include <dev/spi/spi.h>
struct w25q64jv_dev {
    spi_bus bus;
};

int w25q64jv_init(struct w25q64jv_dev *dev, spi_bus bus);
int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len);
int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len);

#endif /* W25Q64JV_H */
