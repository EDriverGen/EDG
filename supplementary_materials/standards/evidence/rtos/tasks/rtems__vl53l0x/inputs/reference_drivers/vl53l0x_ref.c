#include "vl53l0x_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int vl53l0x_transfer(struct vl53l0x_device *dev, struct i2c_msg *msgs,
                            uint32_t nmsgs)
{
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;
    if (dev == 0 || dev->bus_path == 0 || msgs == 0 || nmsgs == 0) {
        return -1;
    }
    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;
    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

static int vl53l0x_read_reg(struct vl53l0x_device *dev, uint8_t reg,
                            uint8_t *buf, uint16_t len)
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
    return vl53l0x_transfer(dev, msgs, 2);
}

static int vl53l0x_write_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t val)
{
    struct i2c_msg msg;
    uint8_t frame[2] = {reg, val};
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.len = sizeof(frame);
    msg.buf = frame;
    return vl53l0x_transfer(dev, &msg, 1);
}

int vl53l0x_init(struct vl53l0x_device *dev, const char *bus_path, uint16_t addr)
{
    if (dev == 0 || bus_path == 0 || addr == 0) {
        return -1;
    }
    dev->bus_path = bus_path;
    dev->addr = addr;
    return 0;
}

int vl53l0x_probe(struct vl53l0x_device *dev)
{
    uint8_t id = 0;
    if (vl53l0x_read_reg(dev, 0xC0, &id, 1) != 0) {
        return -1;
    }
    return id == VL53L0X_MODEL_ID ? 0 : -3;
}

int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm)
{
    uint8_t status = 0;
    uint8_t data[12];
    if (dev == 0 || range_mm == 0) {
        return -1;
    }
    if (vl53l0x_write_reg(dev, 0x00, 0x01) != 0) {
        return -1;
    }
    (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(50));
    if (vl53l0x_read_reg(dev, 0x13, &status, 1) != 0) {
        return -1;
    }
    (void)status;
    if (vl53l0x_read_reg(dev, 0x14, data, sizeof(data)) != 0) {
        return -1;
    }
    *range_mm = (uint16_t)(((uint16_t)data[10] << 8) | data[11]);
    return vl53l0x_write_reg(dev, 0x0B, 0x01);
}
