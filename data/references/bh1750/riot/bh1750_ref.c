/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * BH1750 Light Sensor Driver for RIOT OS
 */
#include "bh1750_ref.h"

#define BH1750_CMD_POWER_DOWN  0x00
#define BH1750_CMD_POWER_ON   0x01
#define BH1750_CMD_RESET      0x07

static int bh1750_write_cmd(struct bh1750_device *dev, uint8_t cmd)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_write_byte(dev->bus, dev->addr, cmd, 0);
    i2c_release(dev->bus);
    return ret;
}

static int bh1750_read_bytes(struct bh1750_device *dev,
                                 uint8_t *buf, uint16_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_read_bytes(dev->bus, dev->addr, buf, len, 0);
    i2c_release(dev->bus);
    return ret;
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

int bh1750_init(struct bh1750_device *dev, i2c_t bus, uint16_t addr)
{
    if (dev == NULL) return -EINVAL;
    if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH) return -EINVAL;
    dev->bus  = bus;
    dev->addr = addr;
    dev->mode = BH1750_ONE_H_RES_MODE;
    return 0;
}

int bh1750_set_mode(struct bh1750_device *dev, uint8_t mode)
{
    if (dev == NULL) return -EINVAL;
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

    if (dev == NULL || raw == NULL) return -EINVAL;

    ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) return ret;
    ret = bh1750_write_cmd(dev, BH1750_CMD_RESET);
    if (ret != 0) return ret;
    ret = bh1750_write_cmd(dev, dev->mode);
    if (ret != 0) return ret;
    ztimer_sleep(ZTIMER_MSEC, bh1750_get_wait_time_ms(dev->mode));

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
