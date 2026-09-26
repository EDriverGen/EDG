#include "at24c256.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include "rtx_os.h"
#include "cmsis_os2.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "cmsis_rtx.h"
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
    if (HAL_I2C_Master_Transmit(hi2c, dev_addr, data, size, 100) != HAL_OK)
        return -1;
    return 0;
}

static int i2c_read(struct at24c256_dev *dev, uint8_t *buf, uint16_t size)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);
    if (HAL_I2C_Master_Receive(hi2c, dev_addr, buf, size, 100) != HAL_OK)
        return -1;
    return 0;
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -1;
    if (addr + len > 32768) return -1;

    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);

    // Dummy write to set address pointer
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);
    if (HAL_I2C_Master_Transmit(hi2c, dev_addr, addr_bytes, 2, 100) != HAL_OK)
        return -1;

    // Sequential read
    if (HAL_I2C_Master_Receive(hi2c, dev_addr, buf, (uint16_t)len, 100) != HAL_OK)
        return -1;

    return 0;
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -1;
    if (addr + len > 32768) return -1;

    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);

    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;

        uint8_t data[2 + 64];
        data[0] = (uint8_t)(current_addr >> 8);
        data[1] = (uint8_t)(current_addr & 0xFF);
        memcpy(&data[2], buf + offset, chunk);

        if (HAL_I2C_Master_Transmit(hi2c, dev_addr, data, (uint16_t)(2 + chunk), 100) != HAL_OK)
            return -1;

        osDelay(AT24C256_WRITE_CYCLE_MS);
        offset += chunk;
    }

    return 0;
}
