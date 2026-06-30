#include "dps310_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int dps310_transfer(struct dps310_device *dev, struct i2c_msg *msgs, uint32_t nmsgs)
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

static int dps310_read_registers(struct dps310_device *dev, uint8_t reg, uint8_t *buffer, uint16_t size)
{
    struct i2c_msg msgs[2];
    if (dev == 0 || buffer == 0 || size == 0) return -1;
    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = size;
    msgs[1].buf = buffer;
    return dps310_transfer(dev, msgs, 2);
}

static int dps310_write_register(struct dps310_device *dev, uint8_t reg, uint8_t value)
{
    struct i2c_msg msg;
    uint8_t frame[2] = {reg, value};
    if (dev == 0) return -1;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.len = sizeof(frame);
    msg.buf = frame;
    return dps310_transfer(dev, &msg, 1);
}

static void dps310_delay_ms(uint32_t ms)
{
    (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(ms));
}

int dps310_init(struct dps310_device *dev, const char *bus_path, uint8_t addr)
{
    if (dev == 0 || bus_path == 0 || addr == 0) return -1;
    dev->bus_path = bus_path;
    dev->addr = addr;
    dev->kT = DPS310_SCALE_FACTOR_1;
    dev->kP = DPS310_SCALE_FACTOR_1;
    return 0;
}

#include "dps310_common_body.h"
