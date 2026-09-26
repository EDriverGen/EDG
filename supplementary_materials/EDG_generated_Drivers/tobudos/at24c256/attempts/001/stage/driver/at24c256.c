#include "at24c256.h"
#include "tos_k.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_i2c.h"

#include "tobudos.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_WRITE_CYCLE_MS 5

int at24c256_init(struct at24c256_dev *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

static int i2c_write(struct at24c256_dev *dev, uint8_t *data, uint16_t size)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, dev_addr, data, size, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int i2c_read(struct at24c256_dev *dev, uint8_t *buf, uint16_t size)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(hi2c, dev_addr, buf, size, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -1;
    if (addr + len > 32768) return -1;
    
    // Dummy write to set address
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);
    if (i2c_write(dev, addr_bytes, 2) != 0) return -1;
    
    // Sequential read
    if (i2c_read(dev, buf, (uint16_t)len) != 0) return -1;
    
    return 0;
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -1;
    if (addr + len > 32768) return -1;
    
    // Page write: up to page boundary
    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + offset;
        size_t page_remaining = AT24C256_PAGE_SIZE - (current_addr % AT24C256_PAGE_SIZE);
        size_t chunk = (len - offset) < page_remaining ? (len - offset) : page_remaining;
        
        // Prepare data: address bytes + chunk
        uint8_t data[2 + AT24C256_PAGE_SIZE];
        data[0] = (uint8_t)(current_addr >> 8);
        data[1] = (uint8_t)(current_addr & 0xFF);
        for (size_t i = 0; i < chunk; i++) {
            data[2 + i] = buf[offset + i];
        }
        
        if (i2c_write(dev, data, (uint16_t)(2 + chunk)) != 0) return -1;
        
        // Wait for write cycle
        tos_sleep_ms(AT24C256_WRITE_CYCLE_MS);
        
        offset += chunk;
    }
    
    return 0;
}
