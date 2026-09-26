#include "vl53l0x.h"
#include <stdint.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define VL53L0X_I2C_ADDR 0x52

static int vl53l0x_read_reg(struct vl53l0x_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
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

int vl53l0x_init(struct vl53l0x_dev *dev, void *bus_handle)
{
    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    os_time_delay(OS_TICKS_PER_SEC / 1000 * 2);
    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw)
{
    uint8_t buf[2];
    int rc;

    rc = vl53l0x_read_reg(dev, 0x00, buf, 2);
    if (rc != 0) return rc;

    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
