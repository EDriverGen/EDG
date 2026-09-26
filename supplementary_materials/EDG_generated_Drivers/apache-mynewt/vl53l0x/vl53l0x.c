#include "vl53l0x.h"
#include <stdint.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define VL53L0X_I2C_ADDR 0x52

int vl53l0x_init(struct vl53l0x_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    os_time_delay(2);
    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw) {
    uint8_t reg = 0x00;
    uint8_t buf[2];
    struct hal_i2c_master_data pdata;
    int rc;

    pdata.address = dev->i2c_addr;
    pdata.len = 1;
    pdata.buffer = &reg;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, 1000, 1);
    if (rc != 0) return rc;

    pdata.address = dev->i2c_addr;
    pdata.len = 2;
    pdata.buffer = buf;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, 1000, 1);
    if (rc != 0) return rc;

    *raw = (int32_t)((buf[0] << 8) | buf[1]);
    return 0;
}
