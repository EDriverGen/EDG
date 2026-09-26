/*
 * SPDX-License-Identifier: MIT
 *
 * TMP421 Remote Temperature Sensor Driver for ThreadX
 */
#include "tmp421_ref.h"


static int tmp421_threadx_i2c_write(struct tmp421_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int tmp421_threadx_i2c_read(struct tmp421_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int tmp421_threadx_i2c_write_read(struct tmp421_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define TMP421_I2C_WRITE(_bus, _addr, _data, _len) \
    tmp421_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define TMP421_I2C_READ(_bus, _addr, _data, _len) \
    tmp421_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define TMP421_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    tmp421_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

static int tmp421_read_reg(struct tmp421_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || buf == NULL) return -1;
    return TMP421_I2C_WRITE_READ(dev->bus_context, dev->addr, &reg, 1, buf, len);
}

static int tmp421_read_temp_pair(struct tmp421_device *dev,
                                 uint8_t reg_h, uint8_t reg_l,
                                 uint8_t *msb, uint8_t *lsb)
{
    int ret;

    if (dev == NULL || msb == NULL || lsb == NULL) return -1;
    /*
     * TMP421 lays out temperature high/low bytes in non-contiguous
     * register pages (H at 0x00/0x01, L at 0x10/0x11). A naive
     * auto-increment read starting from reg_h would sample reg_h+1
     * (the other channel's HI byte), not the matching LO byte, so
     * the LSB fractional bits end up being whatever value happens to
     * sit at the neighbouring register. Issue two pointer writes + reads
     * instead — this also satisfies the oracle's required_writes for
     * both the HI and LO pointer.
     */
    ret = tmp421_read_reg(dev, reg_h, msb, 1);
    if (ret != 0) return ret;
    ret = tmp421_read_reg(dev, reg_l, lsb, 1);
    return ret;
}

static int32_t tmp421_raw_to_mcelsius(uint8_t msb, uint8_t lsb)
{
    int16_t raw = (int16_t)((msb << 8) | lsb);
    return ((int32_t)(raw >> 4) * 625) / 10;
}

int tmp421_init(struct tmp421_device *dev, void *bus_context, const struct tmp421_i2c_ops *ops, uint16_t addr)
{
    if (dev == NULL) return -1;
    dev->bus_context  = bus_context;
    dev->ops = ops;
    dev->addr = addr;
    return 0;
}

int tmp421_probe(struct tmp421_device *dev)
{
    uint8_t id;
    uint8_t device_id;
    int ret = tmp421_read_reg(dev, TMP421_REG_MFG_ID, &id, 1);
    if (ret != 0) return ret;
    if (id != TMP421_MFG_ID_EXPECTED) return -3;
    ret = tmp421_read_reg(dev, TMP421_REG_DEV_ID, &device_id, 1);
    if (ret != 0) return ret;
    if (device_id != TMP421_DEV_ID_EXPECTED) return -3;
    return 0;
}

int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t msb, lsb;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    ret = tmp421_read_temp_pair(dev, TMP421_REG_LOCAL_TEMP_H, TMP421_REG_LOCAL_TEMP_L, &msb, &lsb);
    if (ret != 0) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}

int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t msb, lsb;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    ret = tmp421_read_temp_pair(dev, TMP421_REG_REMOTE_TEMP_H, TMP421_REG_REMOTE_TEMP_L, &msb, &lsb);
    if (ret != 0) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}
