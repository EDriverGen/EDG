#include "vl53l0x.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
#define VL53L0X_I2C_ADDR 0x52
#define VL53L0X_DISTANCE_REG 0x00

static int32_t vl53l0x_write_reg(struct vl53l0x_dev *dev, uint8_t reg) {
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = &reg;
    msg.len = 1;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

static int32_t vl53l0x_read_reg16(struct vl53l0x_dev *dev, uint8_t reg, uint16_t *value) {
    struct I2cMsg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    uint8_t buf[2];
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = 2;
    msgs[1].flags = 0x01; // I2C_FLAG_READ
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return -1;
    }
    *value = ((uint16_t)buf[0] << 8) | buf[1];
    return 0;
}

int32_t vl53l0x_init(struct vl53l0x_dev *dev, DevHandle bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    // Wait for boot (tBOOT max 1.2ms)
    OsalMSleep(2);
    return 0;
}

int32_t vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw) {
    uint16_t distance;
    int32_t ret = vl53l0x_read_reg16(dev, VL53L0X_DISTANCE_REG, &distance);
    if (ret != 0) {
        return -1;
    }
    *raw = (int32_t)distance;
    return 0;
}
