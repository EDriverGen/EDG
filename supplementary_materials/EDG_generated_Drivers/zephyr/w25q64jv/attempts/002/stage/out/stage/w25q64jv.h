#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/drivers/spi.h>

#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_MEMORY_SIZE 8388608

struct device;
struct spi_dt_spec;

int w25q64jv_init(const struct device *dev, const struct spi_dt_spec *spi_cfg);
int w25q64jv_read(const struct device *dev, uint32_t addr, uint8_t *buf, size_t len);
int w25q64jv_write_page(const struct device *dev, uint32_t addr, const uint8_t *buf, size_t len);

#endif /* W25Q64JV_H */