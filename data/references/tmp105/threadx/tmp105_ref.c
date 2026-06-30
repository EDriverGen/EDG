/*
 * SPDX-License-Identifier: MIT
 *
 * TMP105 Temperature Sensor Driver for ThreadX
 */
#include "tmp105_ref.h"


static int tmp105_threadx_i2c_write(struct tmp105_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int tmp105_threadx_i2c_read(struct tmp105_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int tmp105_threadx_i2c_write_read(struct tmp105_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define TMP105_I2C_WRITE(_bus, _addr, _data, _len) \
    tmp105_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define TMP105_I2C_READ(_bus, _addr, _data, _len) \
    tmp105_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define TMP105_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    tmp105_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

static int tmp105_read_reg(struct tmp105_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || buf == NULL) return -1;
    return TMP105_I2C_WRITE_READ(dev->bus_context, dev->addr, &reg, 1, buf, len);
}

static int tmp105_write_reg(struct tmp105_device *dev,
                                uint8_t reg, const uint8_t *data, uint16_t len)
{
    uint8_t buf[16];
    if (dev == NULL || dev->bus_context == NULL || len > 15) return -1;
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) buf[1 + i] = data[i];
    return TMP105_I2C_WRITE(dev->bus_context, dev->addr, buf, len + 1);
}


int tmp105_init(struct tmp105_device *dev, void *bus_context, const struct tmp105_i2c_ops *ops, uint16_t addr)
{
    if (dev == NULL) return -1;
    dev->bus_context  = bus_context;
    dev->ops = ops;
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
    if (dev == NULL || temp_mcelsius == NULL) return -1;
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
    if (res_bits > 3) return -1;
    ret = tmp105_read_reg(dev, TMP105_REG_CONF, &conf, 1);
    if (ret != 0) return ret;
    conf = (conf & ~TMP105_CONF_RES_MASK) | (res_bits << TMP105_CONF_RES_SHIFT);
    return tmp105_write_reg(dev, TMP105_REG_CONF, &conf, 1);
}
