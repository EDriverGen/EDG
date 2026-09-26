#include "bh1750_ref.h"

#define BH1750_CMD_POWER_DOWN 0x00
#define BH1750_CMD_POWER_ON   0x01
#define BH1750_CMD_RESET      0x07

static int mynewt_i2c_write(uint8_t i2c_num, uint8_t addr,
                            const uint8_t *data, uint16_t len)
{
    struct hal_i2c_master_data xfer;
    if (data == 0) {
        return -1;
    }
    xfer.address = addr;
    xfer.len = len;
    xfer.buffer = (uint8_t *)data;
    return hal_i2c_master_write(i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}

static int mynewt_i2c_read(uint8_t i2c_num, uint8_t addr,
                           uint8_t *data, uint16_t len)
{
    struct hal_i2c_master_data xfer;
    if (data == 0) {
        return -1;
    }
    xfer.address = addr;
    xfer.len = len;
    xfer.buffer = data;
    return hal_i2c_master_read(i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}

static int bh1750_write_cmd(struct bh1750_device *dev, uint8_t cmd)
{
    if (dev == 0) {
        return -1;
    }
    return mynewt_i2c_write(dev->i2c_num, dev->addr, &cmd, 1);
}

int bh1750_init(struct bh1750_device *dev, uint8_t i2c_num, uint8_t addr)
{
    if (dev == 0) {
        return -1;
    }
    if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH) {
        return -1;
    }
    hal_i2c_init(i2c_num, 0);
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    dev->mode = BH1750_ONE_H_RES_MODE;
    return 0;
}

int bh1750_probe(struct bh1750_device *dev)
{
    int ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) {
        return ret;
    }
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_DOWN);
}

int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw)
{
    uint8_t data[2];
    int ret;

    if (dev == 0 || raw == 0) {
        return -1;
    }

    ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) {
        return ret;
    }
    ret = bh1750_write_cmd(dev, BH1750_CMD_RESET);
    if (ret != 0) {
        return ret;
    }
    ret = bh1750_write_cmd(dev, dev->mode);
    if (ret != 0) {
        return ret;
    }

    os_time_delay(os_time_ms_to_ticks32(180));

    ret = mynewt_i2c_read(dev->i2c_num, dev->addr, data, 2);
    if (ret != 0) {
        return ret;
    }
    *raw = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    return 0;
}

uint32_t bh1750_raw_to_lux_x100(uint16_t raw)
{
    return (uint32_t)raw * 1000U / 12U;
}

int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100)
{
    uint16_t raw;
    int ret;
    if (lux_x100 == 0) {
        return -1;
    }
    ret = bh1750_read_raw(dev, &raw);
    if (ret != 0) {
        return ret;
    }
    *lux_x100 = bh1750_raw_to_lux_x100(raw);
    return 0;
}
