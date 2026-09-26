#include "ssd1306_ref.h"

static int openharmony_i2c_write(DevHandle bus, uint16_t addr,
                                 const uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;
    msg.addr = addr;
    msg.buf = (uint8_t *)data;
    msg.len = len;
    msg.flags = 0;
    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int openharmony_i2c_read(DevHandle bus, uint16_t addr,
                                uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;
    msg.addr = addr;
    msg.buf = data;
    msg.len = len;
    msg.flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int openharmony_i2c_write_read(DevHandle bus, uint16_t addr,
                                      const uint8_t *wdata, uint16_t wlen,
                                      uint8_t *rdata, uint16_t rlen)
{
    struct I2cMsg msg[2];

    if (bus == NULL || wdata == NULL || rdata == NULL) return -1;
    msg[0].addr = addr;
    msg[0].buf = (uint8_t *)wdata;
    msg[0].len = wlen;
    msg[0].flags = 0;
    msg[1].addr = addr;
    msg[1].buf = rdata;
    msg[1].len = rlen;
    msg[1].flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, msg, 2) == 2) ? 0 : -1;
}

#include <string.h>

static const uint8_t ssd1306_init_cmds[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
    0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
};

static int ssd1306_write_cmd(struct ssd1306_device *dev, uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return openharmony_i2c_write(dev->bus, dev->addr, buf, 2);
}

static int ssd1306_write_data(struct ssd1306_device *dev, const uint8_t *data, int len) {
    uint8_t buf[SSD1306_BUF_SIZE + 1];
    if (len > SSD1306_BUF_SIZE) return -1;
    buf[0] = 0x40;
    memcpy(&buf[1], data, len);
    return openharmony_i2c_write(dev->bus, dev->addr, buf, len + 1);
}

int ssd1306_init(struct ssd1306_device *dev, DevHandle bus, uint8_t addr) {
    if (!dev || !bus) return -1;
    dev->bus = bus; dev->addr = addr;
    memset(dev->buffer, 0, SSD1306_BUF_SIZE);
    for (int i = 0; i < (int)sizeof(ssd1306_init_cmds); i++)
        if (ssd1306_write_cmd(dev, ssd1306_init_cmds[i]) < 0) return -1;
    return 0;
}

void ssd1306_deinit(struct ssd1306_device *dev) {
    if (dev) ssd1306_write_cmd(dev, 0xAE);
}

int ssd1306_probe(struct ssd1306_device *dev) {
    if (!dev) return -1;
    return ssd1306_write_cmd(dev, 0xAE);
}

int ssd1306_clear(struct ssd1306_device *dev) {
    if (!dev) return -1;
    memset(dev->buffer, 0, SSD1306_BUF_SIZE);
    return ssd1306_update(dev);
}

void ssd1306_set_pixel(struct ssd1306_device *dev, uint8_t x, uint8_t y, uint8_t on) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    if (on) dev->buffer[x + (y/8)*SSD1306_WIDTH] |= (1 << (y & 7));
    else    dev->buffer[x + (y/8)*SSD1306_WIDTH] &= ~(1 << (y & 7));
}

int ssd1306_update(struct ssd1306_device *dev) {
    if (!dev) return -1;
    ssd1306_write_cmd(dev, 0x21); ssd1306_write_cmd(dev, 0x00); ssd1306_write_cmd(dev, 0x7F);
    ssd1306_write_cmd(dev, 0x22); ssd1306_write_cmd(dev, 0x00); ssd1306_write_cmd(dev, 0x07);
    return ssd1306_write_data(dev, dev->buffer, SSD1306_BUF_SIZE);
}

int ssd1306_set_contrast(struct ssd1306_device *dev, uint8_t contrast) {
    if (ssd1306_write_cmd(dev, 0x81) < 0) return -1;
    return ssd1306_write_cmd(dev, contrast);
}
