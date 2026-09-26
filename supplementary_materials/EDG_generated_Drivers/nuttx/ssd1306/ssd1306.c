#include "ssd1306.h"
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <arch.h>

#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_CMD_MODE 0x00
#define SSD1306_DATA_MODE 0x40

static int ssd1306_write_cmd(struct ssd1306_dev_s *dev, uint8_t cmd)
{
    struct i2c_msg_s msg;
    uint8_t buf[2];
    buf[0] = SSD1306_CMD_MODE;
    buf[1] = cmd;
    msg.frequency = 100000;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = 2;
    return I2C_TRANSFER(dev->bus, &msg, 1);
}

static int ssd1306_write_cmd2(struct ssd1306_dev_s *dev, uint8_t cmd, uint8_t arg)
{
    struct i2c_msg_s msg;
    uint8_t buf[3];
    buf[0] = SSD1306_CMD_MODE;
    buf[1] = cmd;
    buf[2] = arg;
    msg.frequency = 100000;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = 3;
    return I2C_TRANSFER(dev->bus, &msg, 1);
}

static int ssd1306_write_cmd3(struct ssd1306_dev_s *dev, uint8_t cmd, uint8_t arg1, uint8_t arg2)
{
    struct i2c_msg_s msg;
    uint8_t buf[4];
    buf[0] = SSD1306_CMD_MODE;
    buf[1] = cmd;
    buf[2] = arg1;
    buf[3] = arg2;
    msg.frequency = 100000;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = 4;
    return I2C_TRANSFER(dev->bus, &msg, 1);
}

int ssd1306_init(struct ssd1306_dev_s *dev, struct i2c_master_s *bus)
{
    int ret;
    dev->bus = bus;
    dev->addr = SSD1306_I2C_ADDR;

    /* Power ON sequence: RES# pulse and delay handled externally */
    up_mdelay(100); /* tAF after VCC stable */
    ret = ssd1306_write_cmd(dev, 0xAF);
    if (ret < 0) return ret;

    /* Init sequence */
    ret = ssd1306_write_cmd(dev, 0xAE);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd2(dev, 0xD5, 0x80);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd2(dev, 0xA8, 0x3F);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd2(dev, 0xD3, 0x00);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd(dev, 0x40);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd(dev, 0xA1);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd(dev, 0xC8);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd2(dev, 0xDA, 0x12);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd2(dev, 0x81, 0x7F);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd(dev, 0xA4);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd(dev, 0xA6);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd(dev, 0x2E);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd2(dev, 0x20, 0x00);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
    if (ret < 0) return ret;

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_dev_s *dev, const uint8_t *data, uint16_t len)
{
    int ret;
    /* Set column address range */
    ret = ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
    if (ret < 0) return ret;
    /* Set page address range */
    ret = ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
    if (ret < 0) return ret;

    /* Write data */
    struct i2c_msg_s msg;
    uint8_t *buf = malloc(len + 1);
    if (!buf) return -ENOMEM;
    buf[0] = SSD1306_DATA_MODE;
    memcpy(buf + 1, data, len);
    msg.frequency = 100000;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = len + 1;
    ret = I2C_TRANSFER(dev->bus, &msg, 1);
    free(buf);
    return ret;
}