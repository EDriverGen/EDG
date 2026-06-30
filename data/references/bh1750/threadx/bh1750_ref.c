/*
 * SPDX-License-Identifier: MIT
 *
 * BH1750 Light Sensor Driver for ThreadX
 */
#include "bh1750_ref.h"


static int bh1750_threadx_i2c_write(struct bh1750_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int bh1750_threadx_i2c_read(struct bh1750_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int bh1750_threadx_i2c_write_read(struct bh1750_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define BH1750_I2C_WRITE(_bus, _addr, _data, _len) \
    bh1750_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define BH1750_I2C_READ(_bus, _addr, _data, _len) \
    bh1750_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define BH1750_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    bh1750_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

#define BH1750_CMD_POWER_DOWN  0x00
#define BH1750_CMD_POWER_ON   0x01
#define BH1750_CMD_RESET      0x07

static int bh1750_write_cmd(struct bh1750_device *dev, uint8_t cmd)
{
    /* ThreadX: delegate to platform I2C HAL.
       User must implement BH1750_I2C_WRITE(bus_context, addr, data, len). */
    if (dev == NULL || dev->bus_context == NULL) return -1;
    return BH1750_I2C_WRITE(dev->bus_context, dev->addr, &cmd, 1);
}

static int bh1750_read_bytes(struct bh1750_device *dev,
                                 uint8_t *buf, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || buf == NULL) return -1;
    return BH1750_I2C_READ(dev->bus_context, dev->addr, buf, len);
}

static int bh1750_get_wait_time_ms(uint8_t mode)
{
    switch (mode) {
    case BH1750_CONT_H_RES_MODE:
    case BH1750_CONT_H_RES_MODE2:
    case BH1750_ONE_H_RES_MODE:
    case BH1750_ONE_H_RES_MODE2:
        return 180;
    default:
        return 24;
    }
}

int bh1750_init(struct bh1750_device *dev, void *bus_context, const struct bh1750_i2c_ops *ops, uint16_t addr)
{
    if (dev == NULL) return -1;
    if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH) return -1;
    dev->bus_context  = bus_context;
    dev->ops = ops;
    dev->addr = addr;
    dev->mode = BH1750_ONE_H_RES_MODE;
    return 0;
}

int bh1750_set_mode(struct bh1750_device *dev, uint8_t mode)
{
    if (dev == NULL) return -1;
    dev->mode = mode;
    return 0;
}

int bh1750_probe(struct bh1750_device *dev)
{
    int ret;
    uint8_t cmd = BH1750_CMD_POWER_ON;
    ret = bh1750_write_cmd(dev, cmd);
    if (ret != 0) return ret;
    cmd = BH1750_CMD_POWER_DOWN;
    return bh1750_write_cmd(dev, cmd);
}

int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw)
{
    int ret;
    uint8_t data[2];

    if (dev == NULL || raw == NULL) return -1;

    ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) return ret;
    ret = bh1750_write_cmd(dev, BH1750_CMD_RESET);
    if (ret != 0) return ret;
    ret = bh1750_write_cmd(dev, dev->mode);
    if (ret != 0) return ret;
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * bh1750_get_wait_time_ms(dev->mode) / 1000);

    ret = bh1750_read_bytes(dev, data, 2);
    if (ret != 0) return ret;

    *raw = (uint16_t)((data[0] << 8) | data[1]);
    return 0;
}

uint32_t bh1750_raw_to_lux_x100(uint16_t raw)
{
    return (uint32_t)raw * 100 / 12;
}

int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100)
{
    uint16_t raw;
    int ret = bh1750_read_raw(dev, &raw);
    if (ret != 0) return ret;
    *lux_x100 = bh1750_raw_to_lux_x100(raw);
    return 0;
}
