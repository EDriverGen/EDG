#include "ssd1306.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <dev/i2c/i2c.h>
#include "rtems.h"
static int i2c_write(struct ssd1306_device *dev, const uint8_t *buf, size_t len)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msg.addr = dev->i2c_addr;
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

    fd = open(path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    dev->fd = fd;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    /* Init sequence commands */
    uint8_t cmd_ae[] = {0xAE};
    uint8_t cmd_d5_80[] = {0xD5, 0x80};
    uint8_t cmd_a8_3f[] = {0xA8, 0x3F};
    uint8_t cmd_d3_00[] = {0xD3, 0x00};
    uint8_t cmd_40[] = {0x40};
    uint8_t cmd_a1[] = {0xA1};
    uint8_t cmd_c8[] = {0xC8};
    uint8_t cmd_da_12[] = {0xDA, 0x12};
    uint8_t cmd_81_7f[] = {0x81, 0x7F};
    uint8_t cmd_a4[] = {0xA4};
    uint8_t cmd_a6[] = {0xA6};
    uint8_t cmd_2e[] = {0x2E};
    uint8_t cmd_20_00[] = {0x20, 0x00};
    uint8_t cmd_21_00_7f[] = {0x21, 0x00, 0x7F};
    uint8_t cmd_22_00_07[] = {0x22, 0x00, 0x07};
    uint8_t cmd_af[] = {0xAF};

    if (i2c_write(dev, cmd_ae, sizeof(cmd_ae)) < 0) return -1;
    if (i2c_write(dev, cmd_d5_80, sizeof(cmd_d5_80)) < 0) return -1;
    if (i2c_write(dev, cmd_a8_3f, sizeof(cmd_a8_3f)) < 0) return -1;
    if (i2c_write(dev, cmd_d3_00, sizeof(cmd_d3_00)) < 0) return -1;
    if (i2c_write(dev, cmd_40, sizeof(cmd_40)) < 0) return -1;
    if (i2c_write(dev, cmd_a1, sizeof(cmd_a1)) < 0) return -1;
    if (i2c_write(dev, cmd_c8, sizeof(cmd_c8)) < 0) return -1;
    if (i2c_write(dev, cmd_da_12, sizeof(cmd_da_12)) < 0) return -1;
    if (i2c_write(dev, cmd_81_7f, sizeof(cmd_81_7f)) < 0) return -1;
    if (i2c_write(dev, cmd_a4, sizeof(cmd_a4)) < 0) return -1;
    if (i2c_write(dev, cmd_a6, sizeof(cmd_a6)) < 0) return -1;
    if (i2c_write(dev, cmd_2e, sizeof(cmd_2e)) < 0) return -1;
    if (i2c_write(dev, cmd_20_00, sizeof(cmd_20_00)) < 0) return -1;
    if (i2c_write(dev, cmd_21_00_7f, sizeof(cmd_21_00_7f)) < 0) return -1;
    if (i2c_write(dev, cmd_22_00_07, sizeof(cmd_22_00_07)) < 0) return -1;
    if (i2c_write(dev, cmd_af, sizeof(cmd_af)) < 0) return -1;

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_device *dev, const uint8_t *data, size_t len)
{
    uint8_t cmd_col[] = {0x21, 0x00, 0x7F};
    uint8_t cmd_page[] = {0x22, 0x00, 0x07};

    if (i2c_write(dev, cmd_col, sizeof(cmd_col)) < 0) return -1;
    if (i2c_write(dev, cmd_page, sizeof(cmd_page)) < 0) return -1;
    if (i2c_write(dev, data, len) < 0) return -1;

    return 0;
}
