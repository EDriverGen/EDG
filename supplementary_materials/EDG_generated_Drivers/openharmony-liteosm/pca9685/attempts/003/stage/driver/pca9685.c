#include "pca9685.h"
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>
#include <string.h>

#include "openharmony_liteosm.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08

int pca9685_init(struct pca9685_dev *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return HDF_SUCCESS;
}

static int pca9685_write_then_read(struct pca9685_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    uint8_t reg_buf[1] = {reg};
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = reg_buf;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *value)
{
    if (dev == NULL || value == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t buf[4];
    int ret = pca9685_write_then_read(dev, reg, buf, 4);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    uint16_t off_l = buf[0];
    uint16_t off_h = buf[1] & 0x0F;
    *value = (off_h * 256) + off_l;
    return HDF_SUCCESS;
}
