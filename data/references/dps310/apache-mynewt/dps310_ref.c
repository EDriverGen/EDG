#include "dps310_ref.h"

static int dps310_read_registers(struct dps310_device *dev, uint8_t reg, uint8_t *buffer, uint16_t size)
{
    struct hal_i2c_master_data xfer;
    if (dev == 0 || buffer == 0 || size == 0) return -1;
    xfer.address = dev->addr;
    xfer.len = 1;
    xfer.buffer = &reg;
    if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 0) != 0) return -1;
    xfer.address = dev->addr;
    xfer.len = size;
    xfer.buffer = buffer;
    return hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}

static int dps310_write_register(struct dps310_device *dev, uint8_t reg, uint8_t value)
{
    struct hal_i2c_master_data xfer;
    uint8_t frame[2] = {reg, value};
    if (dev == 0) return -1;
    xfer.address = dev->addr;
    xfer.len = sizeof(frame);
    xfer.buffer = frame;
    return hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}

static void dps310_delay_ms(uint32_t ms)
{
    os_time_delay(os_time_ms_to_ticks32(ms));
}

int dps310_init(struct dps310_device *dev, uint8_t i2c_num, uint8_t addr)
{
    if (dev == 0 || addr == 0) return -1;
    if (hal_i2c_init(i2c_num, 0) != 0) return -1;
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    dev->kT = DPS310_SCALE_FACTOR_1;
    dev->kP = DPS310_SCALE_FACTOR_1;
    return 0;
}

#include "dps310_common_body.h"
