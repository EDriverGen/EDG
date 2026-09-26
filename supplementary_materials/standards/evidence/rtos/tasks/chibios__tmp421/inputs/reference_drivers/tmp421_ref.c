/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Driver for ChibiOS
 */
#include "tmp421_ref.h"

static int tmp421_read_reg(struct tmp421_device *dev,
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


static int32_t tmp421_raw_to_mcelsius(uint8_t msb, uint8_t lsb)
{
    int16_t raw = (int16_t)((msb << 8) | lsb);
    return ((int32_t)(raw >> 4) * 625) / 10;
}

int tmp421_init(struct tmp421_device *dev, I2CDriver *bus, uint16_t addr)
{
    if (dev == NULL) return -1;
    dev->bus  = bus;
    dev->addr = addr;
    return 0;
}

int tmp421_probe(struct tmp421_device *dev)
{
    uint8_t id;
    int ret = tmp421_read_reg(dev, TMP421_REG_MFG_ID, &id, 1);
    if (ret != 0) return ret;
    if (id != TMP421_MFG_ID_EXPECTED) return -3;
    return 0;
}

int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t msb, lsb;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
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
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    ret = tmp421_read_reg(dev, TMP421_REG_REMOTE_TEMP_H, &msb, 1);
    if (ret != 0) return ret;
    ret = tmp421_read_reg(dev, TMP421_REG_REMOTE_TEMP_L, &lsb, 1);
    if (ret != 0) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}
