#include "tmp105_ref.h"

static int tmp105_valid_addr(uint8_t addr)
{
    return addr == TMP105_ADDR_LOW || addr == TMP105_ADDR_HIGH;
}

static int tmp105_read_registers(struct tmp105_device *dev, uint8_t reg,
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

static int tmp105_write_register(struct tmp105_device *dev, uint8_t reg,
                                 uint8_t value)
{
    struct hal_i2c_master_data xfer;
    uint8_t frame[2];
    if (dev == 0) {
        return -1;
    }
    frame[0] = reg;
    frame[1] = value;
    xfer.address = dev->addr;
    xfer.len = sizeof(frame);
    xfer.buffer = frame;
    return hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0
        ? 0 : -1;
}

static int32_t tmp105_raw_to_mcelsius(const uint8_t data[2])
{
    int16_t raw = ((int16_t)(((uint16_t)data[0] << 8) | data[1])) >> 4;
    return (int32_t)raw * 625 / 10;
}

int tmp105_init(struct tmp105_device *dev, uint8_t i2c_num, uint8_t addr)
{
    if (dev == 0 || !tmp105_valid_addr(addr)) {
        return -1;
    }
    if (hal_i2c_init(i2c_num, 0) != 0) {
        return -1;
    }
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    return 0;
}

int tmp105_probe(struct tmp105_device *dev)
{
    uint8_t cfg = 0;
    return tmp105_read_registers(dev, TMP105_REG_CONFIG, &cfg, 1);
}

int tmp105_read_temperature(struct tmp105_device *dev, int32_t *temp_mcelsius)
{
    uint8_t data[2];
    if (temp_mcelsius == 0) {
        return -1;
    }
    if (tmp105_read_registers(dev, TMP105_REG_TEMP, data, sizeof(data)) != 0) {
        return -1;
    }
    *temp_mcelsius = tmp105_raw_to_mcelsius(data);
    return 0;
}

int tmp105_set_resolution(struct tmp105_device *dev, uint8_t bits)
{
    uint8_t cfg = 0;
    uint8_t res_bits;
    if (bits < 9 || bits > 12) {
        return -1;
    }
    if (tmp105_read_registers(dev, TMP105_REG_CONFIG, &cfg, 1) != 0) {
        return -1;
    }
    res_bits = (uint8_t)(bits - 9);
    cfg &= (uint8_t)~(TMP105_CONF_RES_0 | TMP105_CONF_RES_1);
    cfg |= (uint8_t)(res_bits << 5);
    return tmp105_write_register(dev, TMP105_REG_CONFIG, cfg);
}

int tmp105_read_config(struct tmp105_device *dev, uint8_t *config)
{
    if (config == 0) {
        return -1;
    }
    return tmp105_read_registers(dev, TMP105_REG_CONFIG, config, 1);
}
