#include "tmp105_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int tmp105_valid_addr(uint8_t addr)
{
    return addr == TMP105_ADDR_LOW || addr == TMP105_ADDR_HIGH;
}

static int tmp105_read_registers(struct tmp105_device *dev, uint8_t reg,
                                 uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;

    if (dev == 0 || dev->bus_path == 0 || buf == 0 || len == 0) {
        return -1;
    }

    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

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

static int tmp105_write_register(struct tmp105_device *dev, uint8_t reg,
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

static int32_t tmp105_raw_to_mcelsius(const uint8_t data[2])
{
    int16_t raw = ((int16_t)(((uint16_t)data[0] << 8) | data[1])) >> 4;
    return (int32_t)raw * 625 / 10;
}

int tmp105_init(struct tmp105_device *dev, const char *bus_path, uint8_t addr)
{
    if (dev == 0 || bus_path == 0 || !tmp105_valid_addr(addr)) {
        return -1;
    }
    dev->bus_path = bus_path;
    dev->addr = addr;
    return 0;
}

int tmp105_probe(struct tmp105_device *dev)
{
    uint8_t cfg = 0;
    return tmp105_read_registers(dev, TMP105_REG_CONFIG, &cfg, 1);
}

int tmp105_read_temperature(struct tmp105_device *dev, int32_t *temp_mcelsius)
{
    uint8_t data[2];
    if (temp_mcelsius == 0) {
        return -1;
    }
    if (tmp105_read_registers(dev, TMP105_REG_TEMP, data, sizeof(data)) != 0) {
        return -1;
    }
    *temp_mcelsius = tmp105_raw_to_mcelsius(data);
    return 0;
}

int tmp105_set_resolution(struct tmp105_device *dev, uint8_t bits)
{
    uint8_t cfg = 0;
    uint8_t res_bits;
    if (bits < 9 || bits > 12) {
        return -1;
    }
    if (tmp105_read_registers(dev, TMP105_REG_CONFIG, &cfg, 1) != 0) {
        return -1;
    }
    res_bits = (uint8_t)(bits - 9);
    cfg &= (uint8_t)~(TMP105_CONF_RES_0 | TMP105_CONF_RES_1);
    cfg |= (uint8_t)(res_bits << 5);
    return tmp105_write_register(dev, TMP105_REG_CONFIG, cfg);
}

int tmp105_read_config(struct tmp105_device *dev, uint8_t *config)
{
    if (config == 0) {
        return -1;
    }
    return tmp105_read_registers(dev, TMP105_REG_CONFIG, config, 1);
}
