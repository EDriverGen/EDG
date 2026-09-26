#include "ssd1306.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "freertos.h"
#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_I2C_TIMEOUT 100

static int ssd1306_write_cmd(struct ssd1306_dev *dev, uint8_t cmd)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
        SSD1306_I2C_ADDR << 1, &cmd, 1, SSD1306_I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int ssd1306_write_cmd2(struct ssd1306_dev *dev, uint8_t cmd1, uint8_t cmd2)
{
    uint8_t buf[2] = {cmd1, cmd2};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
        SSD1306_I2C_ADDR << 1, buf, 2, SSD1306_I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

static int ssd1306_write_cmd3(struct ssd1306_dev *dev, uint8_t cmd1, uint8_t cmd2, uint8_t cmd3)
{
    uint8_t buf[3] = {cmd1, cmd2, cmd3};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
        SSD1306_I2C_ADDR << 1, buf, 3, SSD1306_I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}

int ssd1306_init(struct ssd1306_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    // Power ON sequence: RES# pulse and delay
    // Assume RES# is controlled externally; we just wait 3us after RES# high
    // Use HAL_Delay(1) as minimum (1ms) since no us delay available
    HAL_Delay(1); // at least 3us, but HAL_Delay minimum is 1ms
    HAL_Delay(100); // wait 100ms after VCC stable before AFh

    // Display OFF
    if (ssd1306_write_cmd(dev, 0xAE) != 0) return -1;

    // Set Display Clock Divide Ratio/Oscillator Frequency
    if (ssd1306_write_cmd2(dev, 0xD5, 0x80) != 0) return -1;

    // Set Multiplex Ratio to 64
    if (ssd1306_write_cmd2(dev, 0xA8, 0x3F) != 0) return -1;

    // Set Display Offset to 0
    if (ssd1306_write_cmd2(dev, 0xD3, 0x00) != 0) return -1;

    // Set Display Start Line to row 0
    if (ssd1306_write_cmd(dev, 0x40) != 0) return -1;

    // Set Segment Re-map: column 127 mapped to SEG0
    if (ssd1306_write_cmd(dev, 0xA1) != 0) return -1;

    // Set COM Output Scan Direction: remapped mode
    if (ssd1306_write_cmd(dev, 0xC8) != 0) return -1;

    // Set COM Pins Hardware Configuration: alternative pin config
    if (ssd1306_write_cmd2(dev, 0xDA, 0x12) != 0) return -1;

    // Set Contrast Control to 0x7F
    if (ssd1306_write_cmd2(dev, 0x81, 0x7F) != 0) return -1;

    // Entire Display ON: resume to RAM content
    if (ssd1306_write_cmd(dev, 0xA4) != 0) return -1;

    // Set Normal Display
    if (ssd1306_write_cmd(dev, 0xA6) != 0) return -1;

    // Deactivate scroll
    if (ssd1306_write_cmd(dev, 0x2E) != 0) return -1;

    // Set Memory Addressing Mode to Horizontal
    if (ssd1306_write_cmd2(dev, 0x20, 0x00) != 0) return -1;

    // Set Column Address Range: 0 to 127
    if (ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F) != 0) return -1;

    // Set Page Address Range: 0 to 7
    if (ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07) != 0) return -1;

    // Display ON
    if (ssd1306_write_cmd(dev, 0xAF) != 0) return -1;

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_dev *dev, const uint8_t *data, uint16_t len)
{
    // Set column address range (0 to 127)
    if (ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F) != 0) return -1;
    // Set page address range (0 to 7)
    if (ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07) != 0) return -1;

    // Write data bytes
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
        SSD1306_I2C_ADDR << 1, (uint8_t *)data, len, SSD1306_I2C_TIMEOUT);
    return (ret == HAL_OK) ? 0 : -1;
}
