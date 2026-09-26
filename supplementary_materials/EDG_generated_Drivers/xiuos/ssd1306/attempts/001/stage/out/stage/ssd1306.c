#include "ssd1306.h"
#include "bus_i2c.h"
#include "transform.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#include "bus.h"
#include "dev_i2c.h"
#include "bus_pin.h"
#define SSD1306_I2C_ADDR 0x3C

static int ssd1306_write_cmd(struct ssd1306_device *dev, uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return PrivWrite(dev->fd, buf, 2);
}

static int ssd1306_write_cmd_bytes(struct ssd1306_device *dev, const uint8_t *cmds, size_t len) {
    uint8_t buf[256];
    if (len > 255) return -EINVAL;
    buf[0] = 0x00;
    for (size_t i = 0; i < len; i++) {
        buf[i+1] = cmds[i];
    }
    return PrivWrite(dev->fd, buf, len + 1);
}

static int ssd1306_write_data(struct ssd1306_device *dev, const uint8_t *data, size_t len) {
    uint8_t buf[1024];
    if (len > 1023) return -EINVAL;
    buf[0] = 0x40;
    for (size_t i = 0; i < len; i++) {
        buf[i+1] = data[i];
    }
    return PrivWrite(dev->fd, buf, len + 1);
}

int ssd1306_init(struct ssd1306_device *dev, struct I2cBus *bus) {
    dev->bus = bus;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) {
        return -1;
    }

    uint16_t i2c_addr = dev->i2c_addr;
    struct PrivIoctlCfg ioctl_cfg;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0) {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -1;
    }

    // Init sequence
    uint8_t cmd;
    cmd = 0xAE; ssd1306_write_cmd(dev, cmd);
    cmd = 0xD5; ssd1306_write_cmd(dev, cmd); cmd = 0x80; ssd1306_write_cmd(dev, cmd);
    cmd = 0xA8; ssd1306_write_cmd(dev, cmd); cmd = 0x3F; ssd1306_write_cmd(dev, cmd);
    cmd = 0xD3; ssd1306_write_cmd(dev, cmd); cmd = 0x00; ssd1306_write_cmd(dev, cmd);
    cmd = 0x40; ssd1306_write_cmd(dev, cmd);
    cmd = 0xA1; ssd1306_write_cmd(dev, cmd);
    cmd = 0xC8; ssd1306_write_cmd(dev, cmd);
    cmd = 0xDA; ssd1306_write_cmd(dev, cmd); cmd = 0x12; ssd1306_write_cmd(dev, cmd);
    cmd = 0x81; ssd1306_write_cmd(dev, cmd); cmd = 0x7F; ssd1306_write_cmd(dev, cmd);
    cmd = 0xA4; ssd1306_write_cmd(dev, cmd);
    cmd = 0xA6; ssd1306_write_cmd(dev, cmd);
    cmd = 0x2E; ssd1306_write_cmd(dev, cmd);
    cmd = 0x20; ssd1306_write_cmd(dev, cmd); cmd = 0x00; ssd1306_write_cmd(dev, cmd);
    cmd = 0x21; ssd1306_write_cmd(dev, cmd); cmd = 0x00; ssd1306_write_cmd(dev, cmd); cmd = 0x7F; ssd1306_write_cmd(dev, cmd);
    cmd = 0x22; ssd1306_write_cmd(dev, cmd); cmd = 0x00; ssd1306_write_cmd(dev, cmd); cmd = 0x07; ssd1306_write_cmd(dev, cmd);
    cmd = 0xAF; ssd1306_write_cmd(dev, cmd);

    PrivTaskDelay(100);

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_device *dev, const uint8_t *data, size_t len) {
    // Set column address range (0x21, 0x00, 0x7F)
    uint8_t col_cmd[3] = {0x21, 0x00, 0x7F};
    ssd1306_write_cmd_bytes(dev, col_cmd, 3);

    // Set page address range (0x22, 0x00, 0x07)
    uint8_t page_cmd[3] = {0x22, 0x00, 0x07};
    ssd1306_write_cmd_bytes(dev, page_cmd, 3);

    // Write data
    return ssd1306_write_data(dev, data, len);
}
