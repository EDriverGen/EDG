#include "pca9685.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08

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

int pca9685_init(struct pca9685_device *dev, void *bus_handle)
{
    (void)bus_handle;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *value)
{
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t wbuf[1] = {reg};
    uint8_t rbuf[4];
    int ret;

    ret = i2c_write_then_read(dev->fd, 0x71, wbuf, 1, rbuf, 4);
    if (ret < 0) {
        return ret;
    }

    uint8_t off_l = rbuf[0];
    uint8_t off_h = rbuf[1];
    *value = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    return 0;
}
