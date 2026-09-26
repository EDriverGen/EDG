#include "vl53l0x.h"
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define VL53L0X_I2C_ADDR 0x52

static int i2c_write_read(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int vl53l0x_init(struct vl53l0x_dev *dev, int bus_handle)
{
    dev->fd = bus_handle;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw)
{
    uint8_t buf[2];
    int ret;

    /* Write register address 0x00 (distance register) then read 2 bytes */
    ret = i2c_write_read(dev->fd, dev->i2c_addr, 0x00, buf, 2);
    if (ret < 0) {
        return -1;
    }

    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
