#include "bme280_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int bme280_transfer(struct bme280_device *dev, struct i2c_msg *msgs, uint32_t nmsgs)
{
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;
    if (dev == 0 || dev->bus_path == 0 || msgs == 0 || nmsgs == 0) return -1;
    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;
    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) return -1;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

static int bme280_read_reg(struct bme280_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;
    return bme280_transfer(dev, msgs, 2);
}

static int bme280_write_reg(struct bme280_device *dev, uint8_t reg, uint8_t val)
{
    struct i2c_msg msg;
    uint8_t frame[2] = {reg, val};
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.len = sizeof(frame);
    msg.buf = frame;
    return bme280_transfer(dev, &msg, 1);
}

static void bme280_delay_ms(uint32_t ms)
{
    (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(ms));
}

int bme280_init(struct bme280_device *dev, const char *bus_path, uint16_t addr)
{
    if (dev == 0 || bus_path == 0 || addr == 0) return -1;
    dev->bus_path = bus_path;
    dev->addr = addr;
    dev->t_fine = 0;
    return 0;
}

#include "bme280_common_body.h"
