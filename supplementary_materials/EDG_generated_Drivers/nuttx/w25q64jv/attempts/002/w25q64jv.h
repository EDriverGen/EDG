#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>

struct spi_dev_s;

struct w25q64jv_dev_s {
    struct spi_dev_s *spi;
};

int w25q64jv_init(struct w25q64jv_dev_s *dev, struct spi_dev_s *spi);
int w25q64jv_read(struct w25q64jv_dev_s *dev, uint32_t addr, uint8_t *buf, size_t len);
int w25q64jv_write(struct w25q64jv_dev_s *dev, uint32_t addr, const uint8_t *buf, size_t len);

#endif /* W25Q64JV_H */