#include "ssd1306.h"
#include "hal.h"
#include "hal_i2c.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SSD1306_I2C_ADDR 0x3C

static void ssd1306_write_cmd(SSD1306Driver *dev, uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, SSD1306_I2C_ADDR, buf, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
}

static void ssd1306_write_cmd2(SSD1306Driver *dev, uint8_t cmd, uint8_t arg) {
    uint8_t buf[3] = {0x00, cmd, arg};
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, SSD1306_I2C_ADDR, buf, 3, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
}

static void ssd1306_write_cmd3(SSD1306Driver *dev, uint8_t cmd, uint8_t arg1, uint8_t arg2) {
    uint8_t buf[4] = {0x00, cmd, arg1, arg2};
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, SSD1306_I2C_ADDR, buf, 4, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
}

void ssd1306_init(SSD1306Driver *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    ssd1306_write_cmd(dev, 0xAE);
    ssd1306_write_cmd2(dev, 0xD5, 0x80);
    ssd1306_write_cmd2(dev, 0xA8, 0x3F);
    ssd1306_write_cmd2(dev, 0xD3, 0x00);
    ssd1306_write_cmd(dev, 0x40);
    ssd1306_write_cmd(dev, 0xA1);
    ssd1306_write_cmd(dev, 0xC8);
    ssd1306_write_cmd2(dev, 0xDA, 0x12);
    ssd1306_write_cmd2(dev, 0x81, 0x7F);
    ssd1306_write_cmd(dev, 0xA4);
    ssd1306_write_cmd(dev, 0xA6);
    ssd1306_write_cmd(dev, 0x2E);
    ssd1306_write_cmd2(dev, 0x20, 0x00);
    ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
    ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
    ssd1306_write_cmd(dev, 0xAF);
}

void ssd1306_write_display_data(SSD1306Driver *dev, const uint8_t *data, size_t len) {
    uint8_t cmd_buf[3];
    i2cAcquireBus((I2CDriver *)dev->bus_handle);

    cmd_buf[0] = 0x00;
    cmd_buf[1] = 0x21;
    cmd_buf[2] = 0x00;
    i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, SSD1306_I2C_ADDR, cmd_buf, 3, NULL, 0, TIME_MS2I(100));

    cmd_buf[1] = 0x22;
    cmd_buf[2] = 0x07;
    i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, SSD1306_I2C_ADDR, cmd_buf, 3, NULL, 0, TIME_MS2I(100));

    i2cReleaseBus((I2CDriver *)dev->bus_handle);

    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    uint8_t *txbuf = (uint8_t *)malloc(len + 1);
    if (txbuf) {
        txbuf[0] = 0x40;
        memcpy(txbuf + 1, data, len);
        i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, SSD1306_I2C_ADDR, txbuf, len + 1, NULL, 0, TIME_MS2I(100));
        free(txbuf);
    }
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
}