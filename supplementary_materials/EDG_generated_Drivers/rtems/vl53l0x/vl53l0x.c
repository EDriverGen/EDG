#include "vl53l0x.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define VL53L0X_I2C_ADDR 0x52

static int i2c_write_read(struct vl53l0x_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    ret = ioctl(dev->fd, I2C_RDWR, &rdwr);
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

    ret = i2c_write_read(dev, 0x00, buf, 2);
    if (ret < 0) {
        return -1;
    }

    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
