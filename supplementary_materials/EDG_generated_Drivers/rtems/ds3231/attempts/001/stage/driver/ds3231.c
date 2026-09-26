#include "ds3231.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define DS3231_ADDR 0x68

static int ds3231_write_then_read(int fd, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t reg_buf = reg;

    msgs[0].addr = DS3231_ADDR;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg_buf;

    msgs[1].addr = DS3231_ADDR;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    if (ioctl(fd, I2C_RDWR, &rdwr) < 0)
        return -EIO;
    return 0;
}

static int ds3231_write(int fd, uint8_t reg, const uint8_t *data, uint16_t len)
{
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t buf[256];

    if (len + 1 > sizeof(buf))
        return -EINVAL;
    buf[0] = reg;
    memcpy(buf + 1, data, len);

    msgs[0].addr = DS3231_ADDR;
    msgs[0].flags = 0;
    msgs[0].len = len + 1;
    msgs[0].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &rdwr) < 0)
        return -EIO;
    return 0;
}

int ds3231_init(struct ds3231_dev *dev, const char *bus_name)
{
    int fd = open(bus_name, O_RDWR);
    if (fd < 0)
        return -EIO;
    dev->fd = fd;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    int ret;

    ret = ds3231_write_then_read(dev->fd, 0x00, buf, 7);
    if (ret < 0)
        return ret;

    t->seconds = ((buf[0] >> 4) * 10) + (buf[0] & 0x0F);
    t->minutes = ((buf[1] >> 4) * 10) + (buf[1] & 0x0F);
    t->hours = ((buf[2] >> 4) * 10) + (buf[2] & 0x0F);
    t->day = buf[3];
    t->date = ((buf[4] >> 4) * 10) + (buf[4] & 0x0F);
    t->month = ((buf[5] >> 4) * 10) + (buf[5] & 0x0F);
    t->year = ((buf[6] >> 4) * 10) + (buf[6] & 0x0F);
    t->weekday = buf[3];

    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t)
{
    uint8_t data[7];

    data[0] = ((t->seconds / 10) << 4) | (t->seconds % 10);
    data[1] = ((t->minutes / 10) << 4) | (t->minutes % 10);
    data[2] = ((t->hours / 10) << 4) | (t->hours % 10);
    data[3] = t->day;
    data[4] = ((t->date / 10) << 4) | (t->date % 10);
    data[5] = ((t->month / 10) << 4) | (t->month % 10);
    data[6] = ((t->year / 10) << 4) | (t->year % 10);

    return ds3231_write(dev->fd, 0x00, data, 7);
}
