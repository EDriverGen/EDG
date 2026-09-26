#include "tmp421_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int tmp421_read_register(struct tmp421_device *dev, uint8_t reg,
                                uint8_t *value)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;

    if (dev == 0 || dev->bus_path == 0 || value == 0) {
        return -1;
    }

    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 1;
    msgs[1].buf = value;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

static int tmp421_write_register(struct tmp421_device *dev, uint8_t reg,
                                 uint8_t value)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t frame[2];
    int fd;
    int ret;

    if (dev == 0 || dev->bus_path == 0) {
        return -1;
    }
    frame[0] = reg;
    frame[1] = value;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.len = sizeof(frame);
    msg.buf = frame;
    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;

    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

static int32_t tmp421_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
    int16_t raw = ((int16_t)(((uint16_t)hi << 8) | lo)) >> 4;
    return (int32_t)raw * 625 / 10;
}

int tmp421_init(struct tmp421_device *dev, const char *bus_path, uint8_t addr)
{
    if (dev == 0 || bus_path == 0 || addr == 0) {
        return -1;
    }
    dev->bus_path = bus_path;
    dev->addr = addr;
    return 0;
}

int tmp421_probe(struct tmp421_device *dev)
{
    uint8_t mfr = 0;
    if (tmp421_read_register(dev, TMP421_REG_MANUFACTURER_ID, &mfr) != 0) {
        return -1;
    }
    return mfr == TMP421_MANUFACTURER_ID_TI ? 0 : -1;
}

int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t hi = 0;
    uint8_t lo = 0;
    if (temp_mcelsius == 0) {
        return -1;
    }
    if (tmp421_read_register(dev, TMP421_REG_LOCAL_TEMP_HI, &hi) != 0 ||
        tmp421_read_register(dev, TMP421_REG_LOCAL_TEMP_LO, &lo) != 0) {
        return -1;
    }
    *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
    return 0;
}

int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t hi = 0;
    uint8_t lo = 0;
    if (temp_mcelsius == 0) {
        return -1;
    }
    if (tmp421_read_register(dev, TMP421_REG_REMOTE_TEMP_HI, &hi) != 0 ||
        tmp421_read_register(dev, TMP421_REG_REMOTE_TEMP_LO, &lo) != 0) {
        return -1;
    }
    *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
    return 0;
}

int tmp421_set_extended_range(struct tmp421_device *dev, int enable)
{
    uint8_t cfg = 0;
    if (tmp421_read_register(dev, TMP421_REG_CONFIG_1, &cfg) != 0) {
        return -1;
    }
    if (enable) {
        cfg |= TMP421_CONFIG1_RANGE;
    } else {
        cfg &= (uint8_t)~TMP421_CONFIG1_RANGE;
    }
    return tmp421_write_register(dev, TMP421_REG_CONFIG_1_WR, cfg);
}
