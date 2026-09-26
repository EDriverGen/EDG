#include "ds3231.h"
#include "rtems.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#define DS3231_ADDR 0x68

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t *wbuf, uint16_t wlen, uint8_t *rbuf, uint16_t rlen)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = wlen;
    msgs[0].buf = wbuf;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = rlen;
    msgs[1].buf = rbuf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int i2c_write(int fd, uint8_t addr, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msg.addr = addr;
    msg.flags = 0;
    msg.len = len;
    msg.buf = buf;

    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int ds3231_init(struct ds3231_dev *dev, const char *bus_name)
{
    int fd = open(bus_name, O_RDWR);
    if (fd < 0) {
        return -EIO;
    }
    dev->fd = fd;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t)
{
    uint8_t wbuf[1] = {0x00};
    uint8_t rbuf[7];
    int ret;

    ret = i2c_write_then_read(dev->fd, DS3231_ADDR, wbuf, 1, rbuf, 7);
    if (ret < 0) {
        return ret;
    }

    t->seconds = ((rbuf[0] >> 4) & 0x0F) * 10 + (rbuf[0] & 0x0F);
    t->minutes = ((rbuf[1] >> 4) & 0x0F) * 10 + (rbuf[1] & 0x0F);
    t->hours = ((rbuf[2] >> 4) & 0x03) * 10 + (rbuf[2] & 0x0F);
    t->day = rbuf[3];
    t->date = ((rbuf[4] >> 4) & 0x03) * 10 + (rbuf[4] & 0x0F);
    t->month = ((rbuf[5] >> 4) & 0x01) * 10 + (rbuf[5] & 0x0F);
    t->year = ((rbuf[6] >> 4) & 0x0F) * 10 + (rbuf[6] & 0x0F);
    t->weekday = rbuf[3];

    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t)
{
    uint8_t buf[8];
    int ret;

    buf[0] = 0x00;
    buf[1] = ((t->seconds / 10) << 4) | (t->seconds % 10);
    buf[2] = ((t->minutes / 10) << 4) | (t->minutes % 10);
    buf[3] = ((t->hours / 10) << 4) | (t->hours % 10);
    buf[4] = t->day;
    buf[5] = ((t->date / 10) << 4) | (t->date % 10);
    buf[6] = ((t->month / 10) << 4) | (t->month % 10);
    buf[7] = ((t->year / 10) << 4) | (t->year % 10);

    ret = i2c_write(dev->fd, DS3231_ADDR, buf, 8);
    if (ret < 0) {
        return ret;
    }
    return 0;
}
