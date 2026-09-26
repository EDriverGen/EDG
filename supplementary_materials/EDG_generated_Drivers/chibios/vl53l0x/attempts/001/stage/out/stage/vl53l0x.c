#include "vl53l0x.h"
#include <string.h>
#include "hal_i2c.h"

#define VL53L0X_I2C_ADDR 0x52
#define VL53L0X_TIMEOUT MS2ST(100)

static int32_t vl53l0x_read_reg(struct vl53l0x_dev *dev, uint8_t reg, uint8_t *buf, size_t len) {
    msg_t msg;
    i2cAcquireBus(dev->bus_handle);
    msg = i2cMasterTransmitTimeout(dev->bus_handle, VL53L0X_I2C_ADDR, &reg, 1, buf, len, VL53L0X_TIMEOUT);
    i2cReleaseBus(dev->bus_handle);
    if (msg != MSG_OK) {
        return -1;
    }
    return 0;
}

int32_t vl53l0x_init(struct vl53l0x_dev *dev, void *bus_handle) {
    dev->bus_handle = (I2CDriver *)bus_handle;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    return 0;
}

int32_t vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw) {
    uint8_t buf[2];
    int32_t ret;
    ret = vl53l0x_read_reg(dev, 0x51, buf, 2);
    if (ret != 0) {
        return ret;
    }
    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}