/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * TMP105 Temperature Sensor Driver for RIOT OS
 */
#include "tmp105_ref.h"

static int tmp105_read_reg(struct tmp105_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_read_regs(dev->bus, dev->addr, reg, buf, len, 0);
    i2c_release(dev->bus);
    return ret;
}

static int tmp105_write_reg(struct tmp105_device *dev,
                                uint8_t reg, const uint8_t *data, uint16_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_write_regs(dev->bus, dev->addr, reg, data, len, 0);
    i2c_release(dev->bus);
    return ret;
}


int tmp105_init(struct tmp105_device *dev, i2c_t bus, uint16_t addr)
{
    if (dev == NULL) return -EINVAL;
    dev->bus  = bus;
    dev->addr = addr;
    return 0;
}

int tmp105_probe(struct tmp105_device *dev)
{
    uint8_t buf[2];
    return tmp105_read_reg(dev, TMP105_REG_TEMP, buf, 2);
}

int tmp105_read_temperature(struct tmp105_device *dev, int32_t *temp_mcelsius)
{
    uint8_t buf[2];
    int16_t raw;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -EINVAL;
    ret = tmp105_read_reg(dev, TMP105_REG_TEMP, buf, 2);
    if (ret != 0) return ret;
    raw = (int16_t)((buf[0] << 8) | buf[1]);
    /* 12-bit resolution: raw >> 4, step = 62.5 mC */
    *temp_mcelsius = ((int32_t)(raw >> 4) * 625) / 10;
    return 0;
}

int tmp105_set_resolution(struct tmp105_device *dev, uint8_t res_bits)
{
    uint8_t conf;
    int ret;
    if (res_bits > 3) return -EINVAL;
    ret = tmp105_read_reg(dev, TMP105_REG_CONF, &conf, 1);
    if (ret != 0) return ret;
    conf = (conf & ~TMP105_CONF_RES_MASK) | (res_bits << TMP105_CONF_RES_SHIFT);
    return tmp105_write_reg(dev, TMP105_REG_CONF, &conf, 1);
}
