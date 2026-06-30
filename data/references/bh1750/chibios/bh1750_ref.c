/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BH1750 Light Sensor Driver for ChibiOS
 */
#include "bh1750_ref.h"

#define BH1750_CMD_POWER_DOWN  0x00
#define BH1750_CMD_POWER_ON   0x01
#define BH1750_CMD_RESET      0x07

static int bh1750_write_cmd(struct bh1750_device *dev, uint8_t cmd)
{
    msg_t ret;
    if (dev == NULL || dev->bus == NULL) return -1;
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, dev->addr,
                                   &cmd, 1, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}

static int bh1750_read_bytes(struct bh1750_device *dev,
                                 uint8_t *buf, uint16_t len)
{
    msg_t ret;
    if (dev == NULL || dev->bus == NULL || buf == NULL) return -1;
    i2cAcquireBus(dev->bus);
    ret = i2cMasterReceiveTimeout(dev->bus, dev->addr,
                                  buf, len, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
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

int bh1750_init(struct bh1750_device *dev, I2CDriver *bus, uint16_t addr)
{
    if (dev == NULL) return -1;
    if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH) return -1;
    dev->bus  = bus;
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
    chThdSleepMilliseconds(bh1750_get_wait_time_ms(dev->mode));

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
