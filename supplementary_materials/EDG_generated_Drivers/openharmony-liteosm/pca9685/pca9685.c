#include "pca9685.h"
#include <stdint.h>
#include <string.h>
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"

#include "openharmony_liteosm.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_READ_ADDR 0x71
#define MODE1_REG 0x00
#define MODE2_REG 0x01
#define LED0_OFF_L_REG 0x08
#define LED0_OFF_H_REG 0x09
#define LED1_ON_L_REG 0x0A
#define LED1_ON_H_REG 0x0B

static int i2c_write_then_read(struct pca9685_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
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

static int i2c_write(struct pca9685_dev *dev, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = len;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

static int i2c_read(struct pca9685_dev *dev, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = len;
    msg.flags = I2C_FLAG_READ;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

int pca9685_init(struct pca9685_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *out)
{
    (void)channel;
    uint8_t reg = LED0_OFF_L_REG;
    uint8_t buf[4];
    int ret;

    // Write register pointer
    ret = i2c_write(dev, &reg, 1);
    if (ret != 0) {
        return -1;
    }

    // Read 4 bytes: LED0_OFF_L, LED0_OFF_H, LED1_ON_L, LED1_ON_H
    ret = i2c_read(dev, buf, 4);
    if (ret != 0) {
        return -1;
    }

    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    uint8_t on_l = buf[2];
    uint8_t on_h = buf[3];

    uint16_t led0_val = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    uint16_t led1_val = ((uint16_t)(on_h & 0x0F) * 256) + on_l;

    // For channel 0 return led0, for channel 1 return led1
    if (channel == 0) {
        *out = led0_val;
    } else {
        *out = led1_val;
    }

    return 0;
}
