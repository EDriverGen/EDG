#include "ssd1306.h"
#include "bus_i2c.h"
#include "transform.h"
#include "dev_i2c.h"
#include "bus.h"
#include "bus_pin.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#define SSD1306_I2C_ADDR 0x3C

static int ssd1306_write_cmd(struct ssd1306_device *dev, uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return PrivWrite(dev->fd, buf, 2);
}

static int ssd1306_write_cmd2(struct ssd1306_device *dev, uint8_t cmd1, uint8_t cmd2) {
    uint8_t buf[3] = {0x00, cmd1, cmd2};
    return PrivWrite(dev->fd, buf, 3);
}

static int ssd1306_write_cmd3(struct ssd1306_device *dev, uint8_t cmd1, uint8_t cmd2, uint8_t cmd3) {
    uint8_t buf[4] = {0x00, cmd1, cmd2, cmd3};
    return PrivWrite(dev->fd, buf, 4);
}

static int ssd1306_write_data(struct ssd1306_device *dev, const uint8_t *data, size_t len) {
    uint8_t *buf = NULL;
    int ret;
    if (len == 0) return 0;
    buf = (uint8_t *)PrivMalloc(len + 1);
    if (!buf) return -ENOMEM;
    buf[0] = 0x40;
    for (size_t i = 0; i < len; i++) buf[i+1] = data[i];
    ret = PrivWrite(dev->fd, buf, len + 1);
    PrivFree(buf);
    return ret;
}

int ssd1306_init(struct ssd1306_device *dev, struct I2cBus *bus_handle) {
    (void)bus_handle;
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = SSD1306_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return dev->fd;

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        return ret;
    }
    dev->i2c_addr = SSD1306_I2C_ADDR;

    ret = ssd1306_write_cmd(dev, 0xAE);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd2(dev, 0xD5, 0x80);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd2(dev, 0xA8, 0x3F);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd2(dev, 0xD3, 0x00);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd(dev, 0x40);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd(dev, 0xA1);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd(dev, 0xC8);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd2(dev, 0xDA, 0x12);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd2(dev, 0x81, 0x7F);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd(dev, 0xA4);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd(dev, 0xA6);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd(dev, 0x2E);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd2(dev, 0x20, 0x00);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
    if (ret < 0) goto fail;
    ret = ssd1306_write_cmd(dev, 0xAF);
    if (ret < 0) goto fail;

    return 0;

fail:
    PrivClose(dev->fd);
    return ret;
}

int ssd1306_write_display_data(struct ssd1306_device *dev, const uint8_t *data, size_t len) {
    int ret;
    ret = ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
    if (ret < 0) return ret;
    ret = ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
    if (ret < 0) return ret;
    ret = ssd1306_write_data(dev, data, len);
    if (ret < 0) return ret;
    return 0;
}