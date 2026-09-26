#include "vl53l0x.h"
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
#define VL53L0X_I2C_ADDR 0x52

static int32_t vl53l0x_write_then_read(struct vl53l0x_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return -1;
    }
    return 0;
}

int32_t vl53l0x_init(struct vl53l0x_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    return 0;
}

int32_t vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw)
{
    uint8_t buf[2];
    int32_t ret;
    // Read reference register 0x51 (16-bit) as a placeholder for distance
    ret = vl53l0x_write_then_read(dev, 0x51, buf, 2);
    if (ret != 0) {
        return -1;
    }
    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
