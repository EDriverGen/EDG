#include "ssd1306.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "freertos.h"
#define SSD1306_I2C_ADDR 0x3C

static int ssd1306_write_cmd(struct ssd1306_dev *dev, uint8_t cmd)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     &cmd, 1, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int ssd1306_write_cmd_bytes(struct ssd1306_dev *dev, const uint8_t *cmds, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     (uint8_t *)cmds, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int ssd1306_init(struct ssd1306_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    // Power ON sequence: RES# low for 3us, high, wait 3us, VCC on, wait 100ms, then AFh
    // Since we don't have GPIO control here, assume RES# and VCC are handled externally.
    // We just send the init commands.

    // init_display sequence
    if (ssd1306_write_cmd(dev, 0xAE) != 0) return -1; // Display OFF
    if (ssd1306_write_cmd_bytes(dev, (uint8_t[]){0xD5, 0x80}, 2) != 0) return -1;
    if (ssd1306_write_cmd_bytes(dev, (uint8_t[]){0xA8, 0x3F}, 2) != 0) return -1;
    if (ssd1306_write_cmd_bytes(dev, (uint8_t[]){0xD3, 0x00}, 2) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x40) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA1) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xC8) != 0) return -1;
    if (ssd1306_write_cmd_bytes(dev, (uint8_t[]){0xDA, 0x12}, 2) != 0) return -1;
    if (ssd1306_write_cmd_bytes(dev, (uint8_t[]){0x81, 0x7F}, 2) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA4) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA6) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x2E) != 0) return -1;
    if (ssd1306_write_cmd_bytes(dev, (uint8_t[]){0x20, 0x00}, 2) != 0) return -1;
    if (ssd1306_write_cmd_bytes(dev, (uint8_t[]){0x21, 0x00, 0x7F}, 3) != 0) return -1;
    if (ssd1306_write_cmd_bytes(dev, (uint8_t[]){0x22, 0x00, 0x07}, 3) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xAF) != 0) return -1; // Display ON

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_dev *dev, const uint8_t *data, uint16_t len)
{
    // Set column address range (0x21, start, end)
    uint8_t col_cmd[] = {0x21, 0x00, 0x7F};
    if (ssd1306_write_cmd_bytes(dev, col_cmd, 3) != 0) return -1;

    // Set page address range (0x22, start, end)
    uint8_t page_cmd[] = {0x22, 0x00, 0x07};
    if (ssd1306_write_cmd_bytes(dev, page_cmd, 3) != 0) return -1;

    // Write data bytes
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     (uint8_t *)data, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}
