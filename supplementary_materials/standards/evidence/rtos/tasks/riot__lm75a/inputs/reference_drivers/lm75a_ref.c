/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * LM75A Temperature Sensor Driver for RIOT OS
 */
#include "lm75a_ref.h"

static int lm75a_read_reg(struct lm75a_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_read_regs(dev->bus, dev->addr, reg, buf, len, 0);
    i2c_release(dev->bus);
    return ret;
}

static int lm75a_write_reg(struct lm75a_device *dev,
                                uint8_t reg, const uint8_t *data, uint16_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_write_regs(dev->bus, dev->addr, reg, data, len, 0);
    i2c_release(dev->bus);
    return ret;
}

static int32_t lm75a_raw_to_mcelsius(int16_t raw)
{
    return (int32_t)(raw >> 5) * 125;
}

int lm75a_init(struct lm75a_device *dev, i2c_t bus, uint16_t addr)
{
    if (dev == NULL) return -EINVAL;
    if (addr < LM75A_ADDR_MIN || addr > LM75A_ADDR_MAX) return -EINVAL;
    dev->bus  = bus;
    dev->addr = addr;
    return 0;
}

int lm75a_probe(struct lm75a_device *dev)
{
    uint8_t conf;
    int ret = lm75a_read_reg(dev, LM75A_REG_CONF, &conf, 1);
    if (ret != 0) return ret;
    if ((conf & 0xE0) != 0) return -ENODEV;
    return 0;
}

int lm75a_read_temperature(struct lm75a_device *dev, int32_t *temp_mcelsius)
{
    uint8_t buf[2];
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -EINVAL;
    ret = lm75a_read_reg(dev, LM75A_REG_TEMP, buf, 2);
    if (ret != 0) return ret;
    *temp_mcelsius = lm75a_raw_to_mcelsius((int16_t)((buf[0] << 8) | buf[1]));
    return 0;
}

int lm75a_read_config(struct lm75a_device *dev, uint8_t *config)
{
    if (dev == NULL || config == NULL) return -EINVAL;
    return lm75a_read_reg(dev, LM75A_REG_CONF, config, 1);
}

int lm75a_write_config(struct lm75a_device *dev, uint8_t config)
{
    return lm75a_write_reg(dev, LM75A_REG_CONF, &config, 1);
}

int lm75a_set_shutdown(struct lm75a_device *dev, bool enable)
{
    uint8_t conf;
    int ret = lm75a_read_config(dev, &conf);
    if (ret != 0) return ret;
    if (enable) conf |= LM75A_CONF_SHUTDOWN;
    else conf &= (uint8_t)~LM75A_CONF_SHUTDOWN;
    return lm75a_write_config(dev, conf);
}
