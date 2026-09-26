#include "tmp105.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_PTR_TEMP 0x00

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t *wbuf, size_t wlen, uint8_t *rbuf, size_t rlen)
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
        return -1;
    }
    return 0;
}

int tmp105_init(struct tmp105_dev *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    dev->fd = fd;
    dev->i2c_addr = TMP105_I2C_ADDR;
    return 0;
}

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    uint8_t wbuf[1] = { TMP105_PTR_TEMP };
    uint8_t rbuf[2];
    int16_t raw12;
    int32_t temp;

    if (i2c_write_then_read(dev->fd, dev->i2c_addr, wbuf, 1, rbuf, 2) < 0) {
        return -1;
    }

    raw12 = (int16_t)((rbuf[0] << 8) | rbuf[1]);
    raw12 >>= 4;
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }

    temp = (int32_t)raw12 * 125 / 2;
    *raw = temp;
    return 0;
}
