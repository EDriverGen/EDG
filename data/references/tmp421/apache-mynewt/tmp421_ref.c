#include "tmp421_ref.h"

static int tmp421_read_register(struct tmp421_device *dev, uint8_t reg,
                                uint8_t *value)
{
    struct hal_i2c_master_data xfer;
    if (dev == 0 || value == 0) {
        return -1;
    }
    xfer.address = dev->addr;
    xfer.len = 1;
    xfer.buffer = &reg;
    if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 0) != 0) {
        return -1;
    }
    xfer.address = dev->addr;
    xfer.len = 1;
    xfer.buffer = value;
    return hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0
        ? 0 : -1;
}

static int tmp421_write_register(struct tmp421_device *dev, uint8_t reg,
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

static int32_t tmp421_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
    int16_t raw = ((int16_t)(((uint16_t)hi << 8) | lo)) >> 4;
    return (int32_t)raw * 625 / 10;
}

int tmp421_init(struct tmp421_device *dev, uint8_t i2c_num, uint8_t addr)
{
    if (dev == 0 || addr == 0) {
        return -1;
    }
    if (hal_i2c_init(i2c_num, 0) != 0) {
        return -1;
    }
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    return 0;
}

int tmp421_probe(struct tmp421_device *dev)
{
    uint8_t mfr = 0;
    if (tmp421_read_register(dev, TMP421_REG_MANUFACTURER_ID, &mfr) != 0) {
        return -1;
    }
    return mfr == TMP421_MANUFACTURER_ID_TI ? 0 : -1;
}

int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t hi = 0;
    uint8_t lo = 0;
    if (temp_mcelsius == 0) {
        return -1;
    }
    if (tmp421_read_register(dev, TMP421_REG_LOCAL_TEMP_HI, &hi) != 0 ||
        tmp421_read_register(dev, TMP421_REG_LOCAL_TEMP_LO, &lo) != 0) {
        return -1;
    }
    *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
    return 0;
}

int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t hi = 0;
    uint8_t lo = 0;
    if (temp_mcelsius == 0) {
        return -1;
    }
    if (tmp421_read_register(dev, TMP421_REG_REMOTE_TEMP_HI, &hi) != 0 ||
        tmp421_read_register(dev, TMP421_REG_REMOTE_TEMP_LO, &lo) != 0) {
        return -1;
    }
    *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
    return 0;
}

int tmp421_set_extended_range(struct tmp421_device *dev, int enable)
{
    uint8_t cfg = 0;
    if (tmp421_read_register(dev, TMP421_REG_CONFIG_1, &cfg) != 0) {
        return -1;
    }
    if (enable) {
        cfg |= TMP421_CONFIG1_RANGE;
    } else {
        cfg &= (uint8_t)~TMP421_CONFIG1_RANGE;
    }
    return tmp421_write_register(dev, TMP421_REG_CONFIG_1_WR, cfg);
}
