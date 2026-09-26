#include "bh1750.h"
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_RESET 0x07
#define BH1750_CMD_CONT_H_RES_MODE 0x10

int bh1750_init(struct bh1750_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;

    struct hal_i2c_master_data pdata;
    pdata.address = dev->i2c_addr;
    pdata.len = 1;
    int rc;

    // Power On
    uint8_t cmd = BH1750_CMD_POWER_ON;
    pdata.buffer = &cmd;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return rc;

    // Reset
    cmd = BH1750_CMD_RESET;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return rc;

    // Start measurement
    cmd = BH1750_CMD_CONT_H_RES_MODE;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return rc;

    // Wait for measurement (max 180ms)
    os_time_delay(OS_TICKS_PER_SEC / 1000 * 180);

    return 0;
}

int bh1750_read(struct bh1750_dev *dev, int32_t *raw) {
    if (!dev || !raw) return -1;

    struct hal_i2c_master_data pdata;
    pdata.address = dev->i2c_addr;
    pdata.len = 1;
    uint8_t cmd = BH1750_CMD_CONT_H_RES_MODE;
    pdata.buffer = &cmd;
    int rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return rc;

    // Wait for measurement (max 180ms)
    os_time_delay(OS_TICKS_PER_SEC / 1000 * 180);

    // Read 2 bytes
    uint8_t buf[2];
    pdata.len = 2;
    pdata.buffer = buf;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return rc;

    uint16_t raw_val = ((uint16_t)buf[0] << 8) | buf[1];
    *raw = ((int32_t)raw_val * 1000) / 1200;

    return 0;
}
