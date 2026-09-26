#include "ds3231_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return (uint8_t)((bcd >> 4) * 10U + (bcd & 0x0FU));
}

static int ds3231_read_reg(struct ds3231_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;
    if (dev == 0 || dev->bus_path == 0 || buf == 0 || len == 0) return -1;
    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;
    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;
    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) return -1;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

int ds3231_init(struct ds3231_device *dev, const char *bus_path, uint16_t addr)
{
    if (dev == 0 || bus_path == 0) return -1;
    dev->bus_path = bus_path;
    dev->addr = addr;
    return 0;
}

int ds3231_probe(struct ds3231_device *dev)
{
    uint8_t val = 0;
    return ds3231_read_reg(dev, 0x00, &val, 1);
}

int ds3231_read_time(struct ds3231_device *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    if (dev == 0 || t == 0) return -1;
    if (ds3231_read_reg(dev, 0x00, buf, 7) != 0) return -1;
    t->seconds = bcd_to_dec(buf[0] & 0x7FU);
    t->minutes = bcd_to_dec(buf[1] & 0x7FU);
    t->hours = bcd_to_dec(buf[2] & 0x3FU);
    t->day = bcd_to_dec(buf[3] & 0x07U);
    t->date = bcd_to_dec(buf[4] & 0x3FU);
    t->month = bcd_to_dec(buf[5] & 0x1FU);
    t->year = bcd_to_dec(buf[6]);
    return 0;
}

int ds3231_read_temperature(struct ds3231_device *dev, int32_t *temp_mcelsius)
{
    uint8_t buf[2];
    int16_t raw;
    if (temp_mcelsius == 0) return -1;
    if (ds3231_read_reg(dev, 0x11, buf, 2) != 0) return -1;
    raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    *temp_mcelsius = ((int32_t)(raw >> 6) * 250);
    return 0;
}
