/*
 * SPDX-License-Identifier: MIT
 *
 * LM75A Temperature Sensor Driver for ThreadX
 */
#include "lm75a_ref.h"


static int lm75a_threadx_i2c_write(struct lm75a_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int lm75a_threadx_i2c_read(struct lm75a_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int lm75a_threadx_i2c_write_read(struct lm75a_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define LM75A_I2C_WRITE(_bus, _addr, _data, _len) \
    lm75a_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define LM75A_I2C_READ(_bus, _addr, _data, _len) \
    lm75a_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define LM75A_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    lm75a_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

static int lm75a_read_reg(struct lm75a_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || buf == NULL) return -1;
    return LM75A_I2C_WRITE_READ(dev->bus_context, dev->addr, &reg, 1, buf, len);
}

static int lm75a_write_reg(struct lm75a_device *dev,
                                uint8_t reg, const uint8_t *data, uint16_t len)
{
    uint8_t buf[16];
    if (dev == NULL || dev->bus_context == NULL || len > 15) return -1;
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) buf[1 + i] = data[i];
    return LM75A_I2C_WRITE(dev->bus_context, dev->addr, buf, len + 1);
}

static int32_t lm75a_raw_to_mcelsius(int16_t raw)
{
    return (int32_t)(raw >> 5) * 125;
}

int lm75a_init(struct lm75a_device *dev, void *bus_context, const struct lm75a_i2c_ops *ops, uint16_t addr)
{
    if (dev == NULL) return -1;
    if (addr < LM75A_ADDR_MIN || addr > LM75A_ADDR_MAX) return -1;
    dev->bus_context  = bus_context;
    dev->ops = ops;
    dev->addr = addr;
    return 0;
}

int lm75a_probe(struct lm75a_device *dev)
{
    uint8_t conf;
    int ret = lm75a_read_reg(dev, LM75A_REG_CONF, &conf, 1);
    if (ret != 0) return ret;
    if ((conf & 0xE0) != 0) return -3;
    return 0;
}

int lm75a_read_temperature(struct lm75a_device *dev, int32_t *temp_mcelsius)
{
    uint8_t buf[2];
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    ret = lm75a_read_reg(dev, LM75A_REG_TEMP, buf, 2);
    if (ret != 0) return ret;
    *temp_mcelsius = lm75a_raw_to_mcelsius((int16_t)((buf[0] << 8) | buf[1]));
    return 0;
}

int lm75a_read_config(struct lm75a_device *dev, uint8_t *config)
{
    if (dev == NULL || config == NULL) return -1;
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
