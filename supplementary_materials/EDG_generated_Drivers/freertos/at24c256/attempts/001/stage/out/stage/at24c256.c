#include "at24c256.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_WRITE_DELAY_MS 5

static int i2c_write(struct at24c256_dev *dev, uint16_t mem_addr, const uint8_t *data, uint16_t size)
{
    uint8_t buffer[2 + size];
    buffer[0] = (uint8_t)(mem_addr >> 8);
    buffer[1] = (uint8_t)(mem_addr & 0xFF);
    for (uint16_t i = 0; i < size; i++) {
        buffer[2 + i] = data[i];
    }
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buffer, 2 + size, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

static int i2c_read(struct at24c256_dev *dev, uint16_t mem_addr, uint8_t *data, uint16_t size)
{
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(mem_addr >> 8);
    addr_bytes[1] = (uint8_t)(mem_addr & 0xFF);
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), addr_bytes, 2, 100) != HAL_OK) {
        return -1;
    }
    if (HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), data, size, 100) != HAL_OK) {
        return -1;
    }
    return 0;
}

int at24c256_init(struct at24c256_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    return i2c_read(dev, addr, buf, (uint16_t)len);
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) {
            chunk = len - offset;
        }
        if (i2c_write(dev, current_addr, buf + offset, (uint16_t)chunk) != 0) {
            return -1;
        }
        HAL_Delay(AT24C256_WRITE_DELAY_MS);
        offset += chunk;
    }
    return 0;
}
