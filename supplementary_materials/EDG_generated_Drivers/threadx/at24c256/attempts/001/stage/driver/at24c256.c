#include "at24c256.h"
#include <stdint.h>
#include <stddef.h>

#include "threadx.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_WRITE_CYCLE_MS 5

static int i2c_write(struct at24c256_dev *dev, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     data, size, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int i2c_read(struct at24c256_dev *dev, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle,
                                                    (uint16_t)(dev->i2c_addr << 1),
                                                    data, size, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int i2c_mem_read(struct at24c256_dev *dev, uint16_t mem_addr, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle,
                                              (uint16_t)(dev->i2c_addr << 1),
                                              mem_addr, I2C_MEMADD_SIZE_16BIT,
                                              data, size, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int at24c256_init(struct at24c256_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0)
        return -1;
    if (addr + len > 32768)
        return -1;
    return i2c_mem_read(dev, addr, buf, (uint16_t)len);
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0)
        return -1;
    if (addr + len > 32768)
        return -1;

    size_t offset = 0;
    while (offset < len) {
        uint16_t current_addr = addr + (uint16_t)offset;
        size_t page_offset = current_addr % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset)
            chunk = len - offset;

        uint8_t tx_buf[2 + 64];
        tx_buf[0] = (uint8_t)(current_addr >> 8);
        tx_buf[1] = (uint8_t)(current_addr & 0xFF);
        for (size_t i = 0; i < chunk; i++) {
            tx_buf[2 + i] = buf[offset + i];
        }

        if (i2c_write(dev, tx_buf, (uint16_t)(2 + chunk)) != 0)
            return -1;

        tx_thread_sleep(5);
        offset += chunk;
    }
    return 0;
}
