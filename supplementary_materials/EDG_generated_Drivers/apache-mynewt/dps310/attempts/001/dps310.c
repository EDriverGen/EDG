#include "dps310.h"
#include <assert.h>
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define DPS310_PSR_B2 0x00
#define DPS310_TMP_B2 0x03
#define DPS310_COEF   0x10
#define DPS310_MEAS_CFG 0x08
#define DPS310_INT_STS 0x0A
#define DPS310_FIFO_STS 0x0B
#define DPS310_ID     0x0D
#define DPS310_RESET  0x0C

static int dps310_i2c_write_then_read(struct dps310_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    struct hal_i2c_master_data pdata;
    int rc;

    pdata.address = dev->i2c_addr;
    pdata.len = 1;
    pdata.buffer = &reg;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICK_PER_SECOND / 10, 1);
    if (rc != 0) return rc;

    pdata.address = dev->i2c_addr;
    pdata.len = len;
    pdata.buffer = buf;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, OS_TICK_PER_SECOND / 10, 1);
    return rc;
}

static int dps310_i2c_write(struct dps310_dev *dev, uint8_t *data, uint16_t len) {
    struct hal_i2c_master_data pdata;
    pdata.address = dev->i2c_addr;
    pdata.len = len;
    pdata.buffer = data;
    return hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICK_PER_SECOND / 10, 1);
}

int dps310_init(struct dps310_dev *dev, void *bus_handle) {
    uint8_t id;
    uint8_t reset_cmd[2] = {DPS310_RESET, 0x09};
    int rc;

    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;

    os_time_delay(OS_TICK_PER_SECOND / 100);

    rc = dps310_i2c_write_then_read(dev, DPS310_ID, &id, 1);
    if (rc != 0) return rc;

    rc = dps310_i2c_write(dev, reset_cmd, 2);
    if (rc != 0) return rc;

    os_time_delay(OS_TICK_PER_SECOND / 100);

    return 0;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw) {
    uint8_t buf[3];
    int rc;

    rc = dps310_i2c_write_then_read(dev, DPS310_PSR_B2, buf, 3);
    if (rc != 0) return rc;

    *pressure_raw = (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
    if (*pressure_raw & 0x800000) {
        *pressure_raw |= 0xFF000000;
    }

    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw) {
    uint8_t buf[3];
    int rc;

    rc = dps310_i2c_write_then_read(dev, DPS310_TMP_B2, buf, 3);
    if (rc != 0) return rc;

    *temp_raw = (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
    if (*temp_raw & 0x800000) {
        *temp_raw |= 0xFF000000;
    }

    return 0;
}
