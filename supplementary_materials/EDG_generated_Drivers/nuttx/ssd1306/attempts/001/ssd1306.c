#include "ssd1306.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <arch.h>

static int ssd1306_i2c_write(struct ssd1306_dev_s *dev, const uint8_t *buf, uint16_t len)
{
    struct i2c_msg_s msg;
    msg.frequency = 100000;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.buffer = (uint8_t *)buf;
    msg.length = len;
    return I2C_TRANSFER(dev->bus, &msg, 1);
}

static int ssd1306_write_command(struct ssd1306_dev_s *dev, uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    return ssd1306_i2c_write(dev, buf, 2);
}

static int ssd1306_write_command_list(struct ssd1306_dev_s *dev, const uint8_t *cmds, uint16_t len)
{
    int ret;
    for (uint16_t i = 0; i < len; i++) {
        ret = ssd1306_write_command(dev, cmds[i]);
        if (ret < 0) return ret;
    }
    return 0;
}

int ssd1306_init(struct ssd1306_dev_s *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = SSD1306_I2C_ADDR;

    /* Power ON sequence: RES# pulse and delay handled externally, then send 0xAF */
    up_mdelay(100); /* tAF after VCC stable */
    int ret = ssd1306_write_command(dev, 0xAF);
    if (ret < 0) return ret;

    /* Init sequence */
    ret = ssd1306_write_command(dev, 0xAE);
    if (ret < 0) return ret;

    ret = ssd1306_write_command_list(dev, (uint8_t[]){0xD5, 0x80}, 2);
    if (ret < 0) return ret;

    ret = ssd1306_write_command_list(dev, (uint8_t[]){0xA8, 0x3F}, 2);
    if (ret < 0) return ret;

    ret = ssd1306_write_command_list(dev, (uint8_t[]){0xD3, 0x00}, 2);
    if (ret < 0) return ret;

    ret = ssd1306_write_command(dev, 0x40);
    if (ret < 0) return ret;

    ret = ssd1306_write_command(dev, 0xA1);
    if (ret < 0) return ret;

    ret = ssd1306_write_command(dev, 0xC8);
    if (ret < 0) return ret;

    ret = ssd1306_write_command_list(dev, (uint8_t[]){0xDA, 0x12}, 2);
    if (ret < 0) return ret;

    ret = ssd1306_write_command_list(dev, (uint8_t[]){0x81, 0x7F}, 2);
    if (ret < 0) return ret;

    ret = ssd1306_write_command(dev, 0xA4);
    if (ret < 0) return ret;

    ret = ssd1306_write_command(dev, 0xA6);
    if (ret < 0) return ret;

    ret = ssd1306_write_command(dev, 0x2E);
    if (ret < 0) return ret;

    ret = ssd1306_write_command_list(dev, (uint8_t[]){0x20, 0x00}, 2);
    if (ret < 0) return ret;

    ret = ssd1306_write_command_list(dev, (uint8_t[]){0x21, 0x00, 0x7F}, 3);
    if (ret < 0) return ret;

    ret = ssd1306_write_command_list(dev, (uint8_t[]){0x22, 0x00, 0x07}, 3);
    if (ret < 0) return ret;

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_dev_s *dev, const uint8_t *data, uint16_t len)
{
    int ret;

    /* Set column address range */
    ret = ssd1306_write_command_list(dev, (uint8_t[]){0x21, 0x00, 0x7F}, 3);
    if (ret < 0) return ret;

    /* Set page address range */
    ret = ssd1306_write_command_list(dev, (uint8_t[]){0x22, 0x00, 0x07}, 3);
    if (ret < 0) return ret;

    /* Write data bytes with control byte 0x40 (D/C#=1) */
    uint8_t *buf = malloc(len + 1);
    if (!buf) return -ENOMEM;
    buf[0] = 0x40;
    memcpy(buf + 1, data, len);
    ret = ssd1306_i2c_write(dev, buf, len + 1);
    free(buf);
    return ret;
}