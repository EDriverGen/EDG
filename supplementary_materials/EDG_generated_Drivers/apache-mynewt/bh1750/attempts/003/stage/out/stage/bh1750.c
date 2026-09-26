#include "bh1750.h"
#include <stdint.h>
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
    int ret;

    // Power On
    pdata.address = dev->i2c_addr;
    uint8_t cmd_on = BH1750_CMD_POWER_ON;
    pdata.buffer = &cmd_on;
    pdata.len = 1;
    ret = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (ret) return ret;

    // Reset
    uint8_t cmd_reset = BH1750_CMD_RESET;
    pdata.buffer = &cmd_reset;
    ret = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (ret) return ret;

    // Start measurement
    uint8_t cmd_start = BH1750_CMD_CONT_H_RES_MODE;
    pdata.buffer = &cmd_start;
    ret = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (ret) return ret;

    // Wait for measurement (max 180ms)
    os_time_delay(OS_TICKS_PER_SEC / 1000 * 180);

    return 0;
}

int bh1750_read(struct bh1750_dev *dev, int32_t *raw) {
    if (!dev || !raw) return -1;

    struct hal_i2c_master_data pdata;
    int ret;

    // Send measurement command (for continuous mode, re-send to trigger read)
    uint8_t cmd = BH1750_CMD_CONT_H_RES_MODE;
    pdata.address = dev->i2c_addr;
    pdata.buffer = &cmd;
    pdata.len = 1;
    ret = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (ret) return ret;

    // Wait for measurement
    os_time_delay(OS_TICKS_PER_SEC / 1000 * 180);

    // Read 2 bytes
    uint8_t buf[2];
    pdata.buffer = buf;
    pdata.len = 2;
    ret = hal_i2c_master_read(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
    if (ret) return ret;

    uint16_t raw_val = ((uint16_t)buf[0] << 8) | buf[1];
    // Convert to milli-lux: (raw * 1000) / 1200
    *raw = ((int32_t)raw_val * 1000) / 1200;

    return 0;
}
