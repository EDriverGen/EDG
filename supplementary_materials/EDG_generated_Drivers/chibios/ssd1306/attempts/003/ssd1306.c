#include "ssd1306.h"
#include "hal.h"
#include "hal_i2c.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SSD1306_I2C_ADDR 0x3C

static void ssd1306_write_cmd(SSD1306Driver *dev, const uint8_t *cmd, size_t len)
{
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, cmd, len, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
}

void ssd1306_init(SSD1306Driver *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    uint8_t cmd;

    cmd = 0xAE; ssd1306_write_cmd(dev, &cmd, 1);
    cmd = 0xD5; uint8_t d5[] = {0xD5, 0x80}; ssd1306_write_cmd(dev, d5, 2);
    cmd = 0xA8; uint8_t a8[] = {0xA8, 0x3F}; ssd1306_write_cmd(dev, a8, 2);
    cmd = 0xD3; uint8_t d3[] = {0xD3, 0x00}; ssd1306_write_cmd(dev, d3, 2);
    cmd = 0x40; ssd1306_write_cmd(dev, &cmd, 1);
    cmd = 0xA1; ssd1306_write_cmd(dev, &cmd, 1);
    cmd = 0xC8; ssd1306_write_cmd(dev, &cmd, 1);
    cmd = 0xDA; uint8_t da[] = {0xDA, 0x12}; ssd1306_write_cmd(dev, da, 2);
    cmd = 0x81; uint8_t c81[] = {0x81, 0x7F}; ssd1306_write_cmd(dev, c81, 2);
    cmd = 0xA4; ssd1306_write_cmd(dev, &cmd, 1);
    cmd = 0xA6; ssd1306_write_cmd(dev, &cmd, 1);
    cmd = 0x2E; ssd1306_write_cmd(dev, &cmd, 1);
    cmd = 0x20; uint8_t c20[] = {0x20, 0x00}; ssd1306_write_cmd(dev, c20, 2);
    cmd = 0x21; uint8_t c21[] = {0x21, 0x00, 0x7F}; ssd1306_write_cmd(dev, c21, 3);
    cmd = 0x22; uint8_t c22[] = {0x22, 0x00, 0x07}; ssd1306_write_cmd(dev, c22, 3);
    cmd = 0xAF; ssd1306_write_cmd(dev, &cmd, 1);
}

void ssd1306_write_display_data(SSD1306Driver *dev, const uint8_t *data, size_t len)
{
    uint8_t cmd;
    cmd = 0x21; uint8_t col[] = {0x21, 0x00, 0x7F}; ssd1306_write_cmd(dev, col, 3);
    cmd = 0x22; uint8_t page[] = {0x22, 0x00, 0x07}; ssd1306_write_cmd(dev, page, 3);

    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, data, len, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
}