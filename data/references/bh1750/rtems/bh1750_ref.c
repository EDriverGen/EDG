#include "bh1750_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define BH1750_CMD_POWER_DOWN 0x00
#define BH1750_CMD_POWER_ON   0x01
#define BH1750_CMD_RESET      0x07

static int rtems_i2c_transfer(const char *bus_path, struct i2c_msg *msgs, uint32_t nmsgs)
{
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;

    if (bus_path == 0 || msgs == 0 || nmsgs == 0) {
        return -1;
    }

    fd = open(bus_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }

    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

static int rtems_i2c_write(const char *bus_path, uint16_t addr,
                           const uint8_t *data, uint16_t len)
{
    struct i2c_msg msg;
    if (data == 0) {
        return -1;
    }
    msg.addr = addr;
    msg.flags = 0;
    msg.len = len;
    msg.buf = (uint8_t *)data;
    return rtems_i2c_transfer(bus_path, &msg, 1);
}

static int rtems_i2c_read(const char *bus_path, uint16_t addr,
                          uint8_t *data, uint16_t len)
{
    struct i2c_msg msg;
    if (data == 0) {
        return -1;
    }
    msg.addr = addr;
    msg.flags = I2C_M_RD;
    msg.len = len;
    msg.buf = data;
    return rtems_i2c_transfer(bus_path, &msg, 1);
}

static int bh1750_write_cmd(struct bh1750_device *dev, uint8_t cmd)
{
    if (dev == 0) {
        return -1;
    }
    return rtems_i2c_write(dev->bus_path, dev->addr, &cmd, 1);
}

int bh1750_init(struct bh1750_device *dev, const char *bus_path, uint16_t addr)
{
    if (dev == 0 || bus_path == 0) {
        return -1;
    }
    if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH) {
        return -1;
    }
    dev->bus_path = bus_path;
    dev->addr = addr;
    dev->mode = BH1750_ONE_H_RES_MODE;
    return 0;
}

int bh1750_probe(struct bh1750_device *dev)
{
    int ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) {
        return ret;
    }
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_DOWN);
}

int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw)
{
    uint8_t data[2];
    int ret;

    if (dev == 0 || raw == 0) {
        return -1;
    }

    ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) {
        return ret;
    }
    ret = bh1750_write_cmd(dev, BH1750_CMD_RESET);
    if (ret != 0) {
        return ret;
    }
    ret = bh1750_write_cmd(dev, dev->mode);
    if (ret != 0) {
        return ret;
    }

    (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(180));

    ret = rtems_i2c_read(dev->bus_path, dev->addr, data, 2);
    if (ret != 0) {
        return ret;
    }
    *raw = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    return 0;
}

uint32_t bh1750_raw_to_lux_x100(uint16_t raw)
{
    return (uint32_t)raw * 1000U / 12U;
}

int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100)
{
    uint16_t raw;
    int ret;
    if (lux_x100 == 0) {
        return -1;
    }
    ret = bh1750_read_raw(dev, &raw);
    if (ret != 0) {
        return ret;
    }
    *lux_x100 = bh1750_raw_to_lux_x100(raw);
    return 0;
}
