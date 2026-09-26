#include "ssd1306.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "cmsis_rtx.h"
#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_TIMEOUT 100

static int ssd1306_write_cmd(struct ssd1306_dev *dev, uint8_t cmd)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(SSD1306_I2C_ADDR << 1),
                                                     &cmd, 1, SSD1306_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int ssd1306_write_cmd2(struct ssd1306_dev *dev, uint8_t cmd1, uint8_t cmd2)
{
    uint8_t buf[2] = {cmd1, cmd2};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(SSD1306_I2C_ADDR << 1),
                                                     buf, 2, SSD1306_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int ssd1306_write_cmd3(struct ssd1306_dev *dev, uint8_t cmd1, uint8_t cmd2, uint8_t cmd3)
{
    uint8_t buf[3] = {cmd1, cmd2, cmd3};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(SSD1306_I2C_ADDR << 1),
                                                     buf, 3, SSD1306_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

int ssd1306_init(struct ssd1306_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    if (ssd1306_write_cmd(dev, 0xAE) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0xD5, 0x80) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0xA8, 0x3F) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0xD3, 0x00) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x40) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA1) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xC8) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0xDA, 0x12) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0x81, 0x7F) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA4) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA6) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x2E) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0x20, 0x00) != 0) return -1;
    if (ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F) != 0) return -1;
    if (ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xAF) != 0) return -1;

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_dev *dev, const uint8_t *data, size_t len)
{
    if (ssd1306_write_cmd2(dev, 0x21, 0x00) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0x22, 0x00) != 0) return -1;

    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(SSD1306_I2C_ADDR << 1),
                                                     (uint8_t *)data, len, SSD1306_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}
