#include "ssd1306.h"
#include "xtimer.h"
#include <stdint.h>
#include <stddef.h>

#include "riot.h"
#define SSD1306_I2C_ADDR 0x3C

static int ssd1306_write_cmd(ssd1306_t *dev, uint8_t cmd)
{
    return i2c_write_bytes(dev->bus, dev->addr, &cmd, 1, 0);
}

static int ssd1306_write_cmd2(ssd1306_t *dev, uint8_t cmd1, uint8_t cmd2)
{
    uint8_t buf[2] = {cmd1, cmd2};
    return i2c_write_bytes(dev->bus, dev->addr, buf, 2, 0);
}

static int ssd1306_write_cmd3(ssd1306_t *dev, uint8_t cmd1, uint8_t cmd2, uint8_t cmd3)
{
    uint8_t buf[3] = {cmd1, cmd2, cmd3};
    return i2c_write_bytes(dev->bus, dev->addr, buf, 3, 0);
}

int ssd1306_init(ssd1306_t *dev, i2c_t bus, uint16_t addr)
{
    int ret;
    dev->bus = bus;
    dev->addr = addr;

    /* Power ON sequence: RES# handled externally, wait 3us after RES# high, then 100ms after VCC stable */
    xtimer_msleep(100);

    /* Display ON command (AFh) after power up */
    ret = ssd1306_write_cmd(dev, 0xAF);
    if (ret != 0) return ret;

    /* Init sequence */
    ret = ssd1306_write_cmd(dev, 0xAE);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd2(dev, 0xD5, 0x80);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd2(dev, 0xA8, 0x3F);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd2(dev, 0xD3, 0x00);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd(dev, 0x40);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd(dev, 0xA1);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd(dev, 0xC8);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd2(dev, 0xDA, 0x12);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd2(dev, 0x81, 0x7F);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd(dev, 0xA4);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd(dev, 0xA6);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd(dev, 0x2E);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd2(dev, 0x20, 0x00);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
    if (ret != 0) return ret;

    ret = ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
    if (ret != 0) return ret;

    return 0;
}

int ssd1306_write_display_data(ssd1306_t *dev, const uint8_t *data, size_t len)
{
    int ret;
    /* Set column address range (0x21, start=0, end=127) */
    ret = ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
    if (ret != 0) return ret;

    /* Set page address range (0x22, start=0, end=7) */
    ret = ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
    if (ret != 0) return ret;

    /* Write data bytes */
    ret = i2c_write_bytes(dev->bus, dev->addr, data, len, 0);
    if (ret != 0) return ret;

    return 0;
}
