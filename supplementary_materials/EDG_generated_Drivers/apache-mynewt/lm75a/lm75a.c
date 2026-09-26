#include "lm75a.h"
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define LM75A_I2C_ADDR 0x48
#define LM75A_TEMP_REG 0x00

int lm75a_init(struct lm75a_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->i2c_addr = LM75A_I2C_ADDR;
    os_time_delay(100);
    return 0;
}

int lm75a_read_temperature(struct lm75a_dev *dev, int32_t *raw) {
    uint8_t cmd = LM75A_TEMP_REG;
    struct hal_i2c_master_data wdata = {
        .address = dev->i2c_addr,
        .len = 1,
        .buffer = &cmd
    };
    int rc = hal_i2c_master_write(dev->i2c_num, &wdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) {
        return -1;
    }

    uint8_t buf[2];
    struct hal_i2c_master_data rdata = {
        .address = dev->i2c_addr,
        .len = 2,
        .buffer = buf
    };
    rc = hal_i2c_master_read(dev->i2c_num, &rdata, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) {
        return -1;
    }

    int16_t raw16 = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw11 = raw16 >> 5;
    if (raw11 & 0x0400) {
        raw11 |= 0xF800;
    }
    *raw = (int32_t)raw11 * 125000;
    return 0;
}
