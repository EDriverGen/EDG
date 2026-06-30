#include "bme280_ref.h"

static int bme280_read_reg(struct bme280_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data xfer;
    if (dev == 0 || buf == 0 || len == 0) return -1;
    xfer.address = (uint8_t)dev->addr;
    xfer.len = 1;
    xfer.buffer = &reg;
    if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 0) != 0) return -1;
    xfer.address = (uint8_t)dev->addr;
    xfer.len = len;
    xfer.buffer = buf;
    return hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}

static int bme280_write_reg(struct bme280_device *dev, uint8_t reg, uint8_t val)
{
    struct hal_i2c_master_data xfer;
    uint8_t frame[2] = {reg, val};
    if (dev == 0) return -1;
    xfer.address = (uint8_t)dev->addr;
    xfer.len = sizeof(frame);
    xfer.buffer = frame;
    return hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}

static void bme280_delay_ms(uint32_t ms)
{
    os_time_delay(os_time_ms_to_ticks32(ms));
}

int bme280_init(struct bme280_device *dev, uint8_t i2c_num, uint16_t addr)
{
    if (dev == 0 || addr == 0) return -1;
    if (hal_i2c_init(i2c_num, 0) != 0) return -1;
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    dev->t_fine = 0;
    return 0;
}

#include "bme280_common_body.h"
