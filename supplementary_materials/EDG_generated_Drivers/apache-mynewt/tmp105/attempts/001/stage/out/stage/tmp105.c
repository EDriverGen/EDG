#include "tmp105.h"
#include <stdint.h>

#include "apache_mynewt.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_REG_TEMP 0x00

int tmp105_init(struct tmp105_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP105_I2C_ADDR;
    os_time_delay(OS_TICKS_PER_SEC / 1000 * 220);
    return 0;
}

int tmp105_read(struct tmp105_dev *dev, int32_t *raw)
{
    uint8_t cmd = TMP105_REG_TEMP;
    uint8_t buf[2];
    struct hal_i2c_master_data pdata;
    int rc;

    pdata.address = dev->i2c_addr;
    pdata.len = 1;
    pdata.buffer = &cmd;
    rc = hal_i2c_master_write(0, &pdata, OS_TICKS_PER_SEC / 1000 * 100, 1);
    if (rc != 0) {
        return -1;
    }

    pdata.address = dev->i2c_addr;
    pdata.len = 2;
    pdata.buffer = buf;
    rc = hal_i2c_master_read(0, &pdata, OS_TICKS_PER_SEC / 1000 * 100, 1);
    if (rc != 0) {
        return -1;
    }

    int16_t raw12 = (int16_t)(((buf[0] << 8) | buf[1]) >> 4);
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }
    *raw = ((int32_t)raw12 * 125) / 2;
    return 0;
}
