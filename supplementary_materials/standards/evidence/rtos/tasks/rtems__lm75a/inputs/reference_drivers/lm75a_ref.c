#include "lm75a_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int lm75a_valid_addr(uint8_t addr)
{
    return addr >= LM75A_ADDR_MIN && addr <= LM75A_ADDR_MAX;
}

static int lm75a_read_registers(struct lm75a_device *dev, uint8_t reg,
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

static int16_t lm75a_unpack_temp(const uint8_t data[2])
{
    uint16_t reg = ((uint16_t)data[0] << 8) | data[1];
    return ((int16_t)reg) >> 5;
}

int lm75a_init(struct lm75a_device *dev, const char *bus_path, uint8_t addr)
{
    if (dev == 0 || bus_path == 0 || !lm75a_valid_addr(addr)) {
        return -1;
    }
    dev->bus_path = bus_path;
    dev->addr = addr;
    return 0;
}

int lm75a_probe(struct lm75a_device *dev)
{
    uint8_t cfg = 0;
    return lm75a_read_registers(dev, LM75A_REG_CONF, &cfg, 1);
}

int lm75a_read_raw(struct lm75a_device *dev, int16_t *raw)
{
    uint8_t data[2];
    if (raw == 0) {
        return -1;
    }
    if (lm75a_read_registers(dev, LM75A_REG_TEMP, data, sizeof(data)) != 0) {
        return -1;
    }
    *raw = lm75a_unpack_temp(data);
    return 0;
}

int32_t lm75a_raw_to_mcelsius(int16_t raw)
{
    return (int32_t)raw * LM75A_TEMP_STEP_MC;
}

int lm75a_read_temp_mcelsius(struct lm75a_device *dev, int32_t *temp_mc)
{
    int16_t raw = 0;
    if (temp_mc == 0) {
        return -1;
    }
    if (lm75a_read_raw(dev, &raw) != 0) {
        return -1;
    }
    *temp_mc = lm75a_raw_to_mcelsius(raw);
    return 0;
}
