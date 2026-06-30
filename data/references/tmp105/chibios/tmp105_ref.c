/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Driver for ChibiOS
 */
#include "tmp105_ref.h"

static int tmp105_read_reg(struct tmp105_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    msg_t ret;
    if (dev == NULL || dev->bus == NULL || buf == NULL) return -1;
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, dev->addr,
                                   &reg, 1, buf, len, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}

static int tmp105_write_reg(struct tmp105_device *dev,
                                uint8_t reg, const uint8_t *data, uint16_t len)
{
    msg_t ret;
    uint8_t buf[16];
    if (dev == NULL || dev->bus == NULL || len > 15) return -1;
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) buf[1 + i] = data[i];
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, dev->addr,
                                   buf, len + 1, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}


int tmp105_init(struct tmp105_device *dev, I2CDriver *bus, uint16_t addr)
{
    if (dev == NULL) return -1;
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
