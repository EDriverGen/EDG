#include "ssd1306.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <dev/i2c/i2c.h>
#include "rtems.h"
#define SSD1306_I2C_ADDR 0x3C

static int i2c_write(struct ssd1306_device *dev, const uint8_t *buf, size_t len)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msg.addr = dev->addr;
    msg.flags = 0;
    msg.len = len;
    msg.buf = (uint8_t *)buf;

    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;

    ret = ioctl(dev->fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int ssd1306_init(struct ssd1306_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    int fd;
    uint8_t cmds[][3] = {
        {0xAE},
        {0xD5, 0x80},
        {0xA8, 0x3F},
        {0xD3, 0x00},
        {0x40},
        {0xA1},
        {0xC8},
        {0xDA, 0x12},
        {0x81, 0x7F},
        {0xA4},
        {0xA6},
        {0x2E},
        {0x20, 0x00},
        {0x21, 0x00, 0x7F},
        {0x22, 0x00, 0x07},
        {0xAF}
    };
    size_t lens[] = {1,2,2,2,1,1,1,2,2,1,1,1,2,3,3,1};
    int i;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    dev->fd = fd;
    dev->addr = SSD1306_I2C_ADDR;

    for (i = 0; i < 16; i++) {
        if (i2c_write(dev, cmds[i], lens[i]) != 0) {
            close(fd);
            dev->fd = -1;
            return -1;
        }
        if (i == 15) {
            usleep(100000);
        }
    }

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_device *dev, const uint8_t *data, size_t len)
{
    uint8_t col_cmd[] = {0x21, 0x00, 0x7F};
    uint8_t page_cmd[] = {0x22, 0x00, 0x07};

    if (i2c_write(dev, col_cmd, sizeof(col_cmd)) != 0) {
        return -1;
    }
    if (i2c_write(dev, page_cmd, sizeof(page_cmd)) != 0) {
        return -1;
    }
    if (i2c_write(dev, data, len) != 0) {
        return -1;
    }
    return 0;
}
