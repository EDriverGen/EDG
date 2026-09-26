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
#define PCA9685_MODE1 0x00
#define PCA9685_MODE1_SLEEP 0x10

static int pca9685_write_reg(struct pca9685_dev *dev, uint8_t reg, uint8_t data)
{
    struct I2cMsg msgs[1];
    uint8_t buf[2] = {reg, data};
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = buf;
    msgs[0].len = 2;
    msgs[0].flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 1);
    return (ret == 1) ? 0 : -1;
}

static int pca9685_read_regs(struct pca9685_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    uint8_t reg_buf = reg;
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg_buf;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    return (ret == 2) ? 0 : -1;
}

int pca9685_init(struct pca9685_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *value)
{
    (void)channel;
    uint8_t buf[4];
    int ret = pca9685_read_regs(dev, PCA9685_LED0_OFF_L, buf, 4);
    if (ret != 0) {
        return -1;
    }
    uint16_t off_l = buf[0];
    uint16_t off_h = buf[1] & 0x0F;
    uint16_t led0_val = (off_h * 256) + off_l;
    off_l = buf[2];
    off_h = buf[3] & 0x0F;
    uint16_t led1_val = (off_h * 256) + off_l;
    if (channel == 0) {
        *value = led0_val;
    } else {
        *value = led1_val;
    }
    return 0;
}
