#include "ssd1306.h"
#include "hal_i2c.h"
#include <string.h>

#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_CTRL_CMD 0x00
#define SSD1306_CTRL_DATA 0x40

static msg_t i2c_write_cmd(SSD1306Driver *dev, const uint8_t *cmds, size_t len) {
    uint8_t buf[256];
    if (len + 1 > sizeof(buf)) return MSG_RESET;
    buf[0] = SSD1306_CTRL_CMD;
    memcpy(buf + 1, cmds, len);
    i2cAcquireBus(dev->i2c);
    msg_t ret = i2cMasterTransmitTimeout(dev->i2c, dev->addr, buf, len + 1, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->i2c);
    return ret;
}

void ssd1306_init(SSD1306Driver *dev, I2CDriver *bus_handle) {
    dev->i2c = bus_handle;
    dev->addr = SSD1306_I2C_ADDR;

    // Power ON sequence: RES# handled externally, just send display ON command
    // But init sequence includes display OFF first, then full init, then display ON
    // According to test plan, init phase includes all commands including 0xAF
    // So we send the full init sequence as per Device IR init_display flow
    // plus power_on write of 0xAF at the end

    // init_display steps
    i2c_write_cmd(dev, (uint8_t[]){0xAE}, 1);
    i2c_write_cmd(dev, (uint8_t[]){0xD5, 0x80}, 2);
    i2c_write_cmd(dev, (uint8_t[]){0xA8, 0x3F}, 2);
    i2c_write_cmd(dev, (uint8_t[]){0xD3, 0x00}, 2);
    i2c_write_cmd(dev, (uint8_t[]){0x40}, 1);
    i2c_write_cmd(dev, (uint8_t[]){0xA1}, 1);
    i2c_write_cmd(dev, (uint8_t[]){0xC8}, 1);
    i2c_write_cmd(dev, (uint8_t[]){0xDA, 0x12}, 2);
    i2c_write_cmd(dev, (uint8_t[]){0x81, 0x7F}, 2);
    i2c_write_cmd(dev, (uint8_t[]){0xA4}, 1);
    i2c_write_cmd(dev, (uint8_t[]){0xA6}, 1);
    i2c_write_cmd(dev, (uint8_t[]){0x2E}, 1);
    i2c_write_cmd(dev, (uint8_t[]){0x20, 0x00}, 2);
    i2c_write_cmd(dev, (uint8_t[]){0x21, 0x00, 0x7F}, 3);
    i2c_write_cmd(dev, (uint8_t[]){0x22, 0x00, 0x07}, 3);
    // power_on: display ON
    i2c_write_cmd(dev, (uint8_t[]){0xAF}, 1);
}

void ssd1306_write_display_data(SSD1306Driver *dev, const uint8_t *data, size_t len) {
    // Set column address range (0x21, start=0, end=127)
    i2c_write_cmd(dev, (uint8_t[]){0x21, 0x00, 0x7F}, 3);
    // Set page address range (0x22, start=0, end=7)
    i2c_write_cmd(dev, (uint8_t[]){0x22, 0x00, 0x07}, 3);

    // Write data bytes with control byte 0x40
    uint8_t buf[1025];
    if (len > 1024) len = 1024;
    buf[0] = SSD1306_CTRL_DATA;
    memcpy(buf + 1, data, len);
    i2cAcquireBus(dev->i2c);
    i2cMasterTransmitTimeout(dev->i2c, dev->addr, buf, len + 1, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->i2c);
}