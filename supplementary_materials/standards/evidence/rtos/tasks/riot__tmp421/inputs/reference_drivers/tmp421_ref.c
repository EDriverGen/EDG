/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * TMP421 Remote Temperature Sensor Driver for RIOT OS
 */
#include "tmp421_ref.h"

static int tmp421_read_reg(struct tmp421_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_read_regs(dev->bus, dev->addr, reg, buf, len, 0);
    i2c_release(dev->bus);
    return ret;
}


static int32_t tmp421_raw_to_mcelsius(uint8_t msb, uint8_t lsb)
{
    int16_t raw = (int16_t)((msb << 8) | lsb);
    return ((int32_t)(raw >> 4) * 625) / 10;
}

int tmp421_init(struct tmp421_device *dev, i2c_t bus, uint16_t addr)
{
    if (dev == NULL) return -EINVAL;
    dev->bus  = bus;
    dev->addr = addr;
    return 0;
}

int tmp421_probe(struct tmp421_device *dev)
{
    uint8_t id;
    int ret = tmp421_read_reg(dev, TMP421_REG_MFG_ID, &id, 1);
    if (ret != 0) return ret;
    if (id != TMP421_MFG_ID_EXPECTED) return -ENODEV;
    return 0;
}

int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t msb, lsb;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -EINVAL;
    ret = tmp421_read_reg(dev, TMP421_REG_LOCAL_TEMP_H, &msb, 1);
    if (ret != 0) return ret;
    ret = tmp421_read_reg(dev, TMP421_REG_LOCAL_TEMP_L, &lsb, 1);
    if (ret != 0) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}

int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t msb, lsb;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -EINVAL;
    ret = tmp421_read_reg(dev, TMP421_REG_REMOTE_TEMP_H, &msb, 1);
    if (ret != 0) return ret;
    ret = tmp421_read_reg(dev, TMP421_REG_REMOTE_TEMP_L, &lsb, 1);
    if (ret != 0) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}
