#include "at24c256_ref.h"

int at24c256_init(struct at24c256_device *dev, I2C_HandleTypeDef *bus, uint16_t addr)
{
    if (dev == 0 || bus == 0) return -1;
    if (HAL_I2C_Init(bus) != HAL_OK) return -1;
    dev->bus = bus;
    dev->addr = addr;
    return 0;
}

int at24c256_probe(struct at24c256_device *dev)
{
    uint8_t data = 0;
    return at24c256_read(dev, 0, &data, 1);
}

int at24c256_write(struct at24c256_device *dev, uint16_t mem_addr, const uint8_t *data, uint16_t len)
{
    uint16_t offset = 0;
    if (dev == 0 || dev->bus == 0 || data == 0) return -1;
    while (offset < len) {
        uint16_t page_rem = (uint16_t)(AT24C256_PAGE_SIZE - ((mem_addr + offset) % AT24C256_PAGE_SIZE));
        uint16_t chunk = (uint16_t)((len - offset) < page_rem ? (len - offset) : page_rem);
        if (HAL_I2C_Mem_Write(dev->bus, (uint16_t)(dev->addr << 1),
                              (uint16_t)(mem_addr + offset),
                              I2C_MEMADD_SIZE_16BIT,
                              (uint8_t *)&data[offset], chunk, 100) != HAL_OK) {
            return -1;
        }
        HAL_Delay(5);
        offset = (uint16_t)(offset + chunk);
    }
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    if (dev == 0 || dev->bus == 0 || data == 0) return -1;
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(dev->addr << 1),
                            mem_addr, I2C_MEMADD_SIZE_16BIT,
                            data, len, 100) == HAL_OK ? 0 : -1;
}
