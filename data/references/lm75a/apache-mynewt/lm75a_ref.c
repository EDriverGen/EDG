#include "lm75a_ref.h"

static int lm75a_valid_addr(uint8_t addr)
{
    return addr >= LM75A_ADDR_MIN && addr <= LM75A_ADDR_MAX;
}

static int lm75a_read_registers(struct lm75a_device *dev, uint8_t reg,
                                uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data xfer;
    if (dev == 0 || buf == 0 || len == 0) {
        return -1;
    }

    xfer.address = dev->addr;
    xfer.len = 1;
    xfer.buffer = &reg;
    if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 0) != 0) {
        return -1;
    }

    xfer.address = dev->addr;
    xfer.len = len;
    xfer.buffer = buf;
    return hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0
        ? 0 : -1;
}

static int16_t lm75a_unpack_temp(const uint8_t data[2])
{
    uint16_t reg = ((uint16_t)data[0] << 8) | data[1];
    return ((int16_t)reg) >> 5;
}

int lm75a_init(struct lm75a_device *dev, uint8_t i2c_num, uint8_t addr)
{
    if (dev == 0 || !lm75a_valid_addr(addr)) {
        return -1;
    }
    if (hal_i2c_init(i2c_num, 0) != 0) {
        return -1;
    }
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    return 0;
}

int lm75a_probe(struct lm75a_device *dev)
{
    uint8_t cfg = 0;
    return lm75a_read_registers(dev, LM75A_REG_CONF, &cfg, 1);
}

int lm75a_read_raw(struct lm75a_device *dev, int16_t *raw)
{
    uint8_t data[2];
    if (raw == 0) {
        return -1;
    }
    if (lm75a_read_registers(dev, LM75A_REG_TEMP, data, sizeof(data)) != 0) {
        return -1;
    }
    *raw = lm75a_unpack_temp(data);
    return 0;
}

int32_t lm75a_raw_to_mcelsius(int16_t raw)
{
    return (int32_t)raw * LM75A_TEMP_STEP_MC;
}

int lm75a_read_temp_mcelsius(struct lm75a_device *dev, int32_t *temp_mc)
{
    int16_t raw = 0;
    if (temp_mc == 0) {
        return -1;
    }
    if (lm75a_read_raw(dev, &raw) != 0) {
        return -1;
    }
    *temp_mc = lm75a_raw_to_mcelsius(raw);
    return 0;
}
