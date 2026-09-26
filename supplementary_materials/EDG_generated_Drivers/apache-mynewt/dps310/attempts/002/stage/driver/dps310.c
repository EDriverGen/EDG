#include "dps310.h"
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define DPS310_I2C_ADDR 0x77
#define DPS310_PSR_B2 0x00
#define DPS310_TMP_B2 0x03
#define DPS310_COEF 0x10
#define DPS310_MEAS_CFG 0x08
#define DPS310_INT_STS 0x0A
#define DPS310_FIFO_STS 0x0B
#define DPS310_ID 0x0D
#define DPS310_RESET 0x0C
#define DPS310_RESET_PATTERN 0x09

static int dps310_i2c_write_then_read(struct dps310_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data pdata;
    int rc;

    pdata.address = dev->i2c_addr;
    pdata.len = 1;
    pdata.buffer = &reg;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
    if (rc != 0) return rc;

    pdata.address = dev->i2c_addr;
    pdata.len = len;
    pdata.buffer = buf;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
    return rc;
}

static int dps310_i2c_write(struct dps310_dev *dev, uint8_t reg, uint8_t data)
{
    struct hal_i2c_master_data pdata;
    uint8_t buf[2] = {reg, data};
    pdata.address = dev->i2c_addr;
    pdata.len = 2;
    pdata.buffer = buf;
    return hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
}

int dps310_init(struct dps310_dev *dev, void *bus_handle)
{
    uint8_t id;
    int rc;

    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;

    os_time_delay(OS_TICKS_PER_SEC * 12 / 1000);

    rc = dps310_i2c_write_then_read(dev, DPS310_ID, &id, 1);
    if (rc != 0) return rc;

    rc = dps310_i2c_write(dev, DPS310_RESET, DPS310_RESET_PATTERN);
    if (rc != 0) return rc;

    os_time_delay(OS_TICKS_PER_SEC * 40 / 1000);

    {
        uint8_t coef_buf[18];
        rc = dps310_i2c_write_then_read(dev, DPS310_COEF, coef_buf, 18);
        if (rc != 0) return rc;
    }

    return 0;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    uint8_t buf[3];
    int rc;
    uint32_t raw;

    rc = dps310_i2c_write_then_read(dev, DPS310_PSR_B2, buf, 3);
    if (rc != 0) return rc;

    raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    if (raw & 0x800000) {
        *pressure_raw = (int32_t)(raw | 0xFF000000);
    } else {
        *pressure_raw = (int32_t)raw;
    }

    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    uint8_t buf[3];
    int rc;
    uint32_t raw;

    rc = dps310_i2c_write_then_read(dev, DPS310_TMP_B2, buf, 3);
    if (rc != 0) return rc;

    raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    if (raw & 0x800000) {
        *temp_raw = (int32_t)(raw | 0xFF000000);
    } else {
        *temp_raw = (int32_t)raw;
    }

    return 0;
}
