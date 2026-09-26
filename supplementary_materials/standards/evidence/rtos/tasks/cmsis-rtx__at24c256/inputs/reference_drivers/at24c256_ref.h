#ifndef AT24C256_CMSIS_RTX_REF_H
#define AT24C256_CMSIS_RTX_REF_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define AT24C256_ADDR_DEFAULT 0x50U
#define AT24C256_PAGE_SIZE    64U
#define AT24C256_MEM_SIZE     32768U

struct at24c256_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
};

int at24c256_init(struct at24c256_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int at24c256_probe(struct at24c256_device *dev);
int at24c256_write(struct at24c256_device *dev, uint16_t mem_addr, const uint8_t *data, uint16_t len);
int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len);

#endif
