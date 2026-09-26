#include "emc1413_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int emc1413_read_register(struct emc1413_device *dev, uint8_t reg,
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

static int emc1413_write_register(struct emc1413_device *dev, uint8_t reg,
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

static int emc1413_channel_regs(enum emc1413_channel channel, uint8_t *hi, uint8_t *lo)
{
    if (hi == 0 || lo == 0) {
        return -1;
    }
    switch (channel) {
    case EMC1413_CH_INTERNAL:
        *hi = EMC1413_REG_INTERNAL_TEMP_HI;
        *lo = EMC1413_REG_INTERNAL_TEMP_LO;
        return 0;
    case EMC1413_CH_EXTERNAL_1:
        *hi = EMC1413_REG_EXT1_TEMP_HI;
        *lo = EMC1413_REG_EXT1_TEMP_LO;
        return 0;
    case EMC1413_CH_EXTERNAL_2:
        *hi = EMC1413_REG_EXT2_TEMP_HI;
        *lo = EMC1413_REG_EXT2_TEMP_LO;
        return 0;
    default:
        return -1;
    }
}

static int32_t emc1413_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
    int32_t integer = (int8_t)hi;
    int32_t frac = (int32_t)(((lo >> 5) & 0x07U) * 125U);
    return integer * 1000 + frac;
}

int emc1413_init(struct emc1413_device *dev, const char *bus_path, uint8_t addr)
{
    if (dev == 0 || bus_path == 0 || addr == 0) {
        return -1;
    }
    dev->bus_path = bus_path;
    dev->addr = addr;
    return 0;
}

int emc1413_probe(struct emc1413_device *dev)
{
    uint8_t mfr = 0;
    if (emc1413_read_register(dev, EMC1413_REG_MANUFACTURER_ID, &mfr) != 0) {
        return -1;
    }
    return mfr == EMC1413_MANUFACTURER_ID ? 0 : -1;
}

int emc1413_read_temperature(struct emc1413_device *dev, enum emc1413_channel channel,
                             int32_t *temp_mcelsius)
{
    uint8_t hi_reg = 0;
    uint8_t lo_reg = 0;
    uint8_t hi = 0;
    uint8_t lo = 0;
    if (temp_mcelsius == 0 || emc1413_channel_regs(channel, &hi_reg, &lo_reg) != 0) {
        return -1;
    }
    if (emc1413_read_register(dev, hi_reg, &hi) != 0 ||
        emc1413_read_register(dev, lo_reg, &lo) != 0) {
        return -1;
    }
    *temp_mcelsius = emc1413_raw_to_mcelsius(hi, lo);
    return 0;
}

int emc1413_set_extended_range(struct emc1413_device *dev, int enable)
{
    uint8_t cfg = 0;
    if (emc1413_read_register(dev, EMC1413_REG_CONFIG, &cfg) != 0) {
        return -1;
    }
    if (enable) {
        cfg |= EMC1413_CONFIG_RANGE;
    } else {
        cfg &= (uint8_t)~EMC1413_CONFIG_RANGE;
    }
    return emc1413_write_register(dev, EMC1413_REG_CONFIG, cfg);
}
