#ifndef AT24C256_RTEMS_REF_H
#define AT24C256_RTEMS_REF_H

#include <stdint.h>
#include <rtems.h>

#define AT24C256_ADDR_DEFAULT 0x50U
#define AT24C256_PAGE_SIZE    64U
#define AT24C256_MEM_SIZE     32768U

struct at24c256_device {
    const char *bus_path;
    uint16_t addr;
};

int at24c256_init(struct at24c256_device *dev, const char *bus_path, uint16_t addr);
int at24c256_probe(struct at24c256_device *dev);
int at24c256_write(struct at24c256_device *dev, uint16_t mem_addr, const uint8_t *data, uint16_t len);
int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len);

#endif
