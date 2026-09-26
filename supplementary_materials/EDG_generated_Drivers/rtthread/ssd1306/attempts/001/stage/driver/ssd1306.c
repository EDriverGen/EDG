#include "ssd1306.h"
#include "rtdevice.h"
#include <stdint.h>
#include <string.h>

#include <drivers/dev_i2c.h>
#include "rtthread.h"
static int ssd1306_write_cmd(struct ssd1306_device *dev, uint8_t cmd)
{
    struct rt_i2c_msg msg;
    uint8_t buf[2];
    buf[0] = 0x00; // control byte: Co=0, D/C#=0 (command)
    buf[1] = cmd;
    msg.addr = dev->i2c_addr;
    msg.flags = 0; // write
    msg.len = 2;
    msg.buf = buf;
    rt_ssize_t ret = rt_i2c_transfer(dev->bus, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

static int ssd1306_write_cmd_with_data(struct ssd1306_device *dev, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    struct rt_i2c_msg msg;
    uint8_t buf[2 + 256];
    uint16_t i;
    buf[0] = 0x00; // control byte: command
    buf[1] = cmd;
    for (i = 0; i < len; i++) {
        buf[2 + i] = data[i];
    }
    msg.addr = dev->i2c_addr;
    msg.flags = 0;
    msg.len = 2 + len;
    msg.buf = buf;
    rt_ssize_t ret = rt_i2c_transfer(dev->bus, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

static int ssd1306_write_data(struct ssd1306_device *dev, const uint8_t *data, uint16_t len)
{
    struct rt_i2c_msg msg;
    uint8_t buf[1 + 256];
    uint16_t i;
    buf[0] = 0x40; // control byte: Co=0, D/C#=1 (data)
    for (i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }
    msg.addr = dev->i2c_addr;
    msg.flags = 0;
    msg.len = 1 + len;
    msg.buf = buf;
    rt_ssize_t ret = rt_i2c_transfer(dev->bus, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

int ssd1306_init(struct ssd1306_device *dev, struct rt_i2c_bus_device *bus)
{
    if (!dev || !bus) return -1;
    dev->bus = bus;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    // Init sequence per Section B
    if (ssd1306_write_cmd(dev, 0xAE) != 0) return -1; // Display OFF
    if (ssd1306_write_cmd_with_data(dev, 0xD5, (uint8_t[]){0x80}, 1) != 0) return -1;
    if (ssd1306_write_cmd_with_data(dev, 0xA8, (uint8_t[]){0x3F}, 1) != 0) return -1;
    if (ssd1306_write_cmd_with_data(dev, 0xD3, (uint8_t[]){0x00}, 1) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x40) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA1) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xC8) != 0) return -1;
    if (ssd1306_write_cmd_with_data(dev, 0xDA, (uint8_t[]){0x12}, 1) != 0) return -1;
    if (ssd1306_write_cmd_with_data(dev, 0x81, (uint8_t[]){0x7F}, 1) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA4) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA6) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x2E) != 0) return -1;
    if (ssd1306_write_cmd_with_data(dev, 0x20, (uint8_t[]){0x00}, 1) != 0) return -1;
    if (ssd1306_write_cmd_with_data(dev, 0x21, (uint8_t[]){0x00, 0x7F}, 2) != 0) return -1;
    if (ssd1306_write_cmd_with_data(dev, 0x22, (uint8_t[]){0x00, 0x07}, 2) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xAF) != 0) return -1; // Display ON

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_device *dev, const uint8_t *data, uint16_t len)
{
    if (!dev || !data || len == 0) return -1;

    // Set column address range (0x21, start=0, end=127)
    if (ssd1306_write_cmd_with_data(dev, 0x21, (uint8_t[]){0x00, 0x7F}, 2) != 0) return -1;
    // Set page address range (0x22, start=0, end=7)
    if (ssd1306_write_cmd_with_data(dev, 0x22, (uint8_t[]){0x00, 0x07}, 2) != 0) return -1;
    // Write data
    if (ssd1306_write_data(dev, data, len) != 0) return -1;

    return 0;
}
