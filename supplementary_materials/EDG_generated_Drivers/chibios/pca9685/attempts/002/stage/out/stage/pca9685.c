#include "pca9685.h"
#include "hal.h"
#include "hal_i2c.h"
#include <string.h>

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_READ_ADDR 0x71
#define MODE1_REG 0x00
#define MODE2_REG 0x01
#define LED0_OFF_L_REG 0x08
#define LED0_OFF_H_REG 0x09
#define LED1_OFF_L_REG 0x0A
#define LED1_OFF_H_REG 0x0B

static msg_t i2c_write_then_read(struct pca9685_device *dev, uint8_t reg, uint8_t *rxbuf, size_t rxbytes) {
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, &reg, 1, rxbuf, rxbytes, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return ret;
}

static msg_t i2c_write(struct pca9685_device *dev, uint8_t *txbuf, size_t txbytes) {
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, txbuf, txbytes, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return ret;
}

static msg_t i2c_read(struct pca9685_device *dev, uint8_t *rxbuf, size_t rxbytes) {
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterReceiveTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, rxbuf, rxbytes, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return ret;
}

void pca9685_init(struct pca9685_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
}

uint16_t pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *out) {
    uint8_t reg;
    uint8_t buf[4];
    uint8_t off_l, off_h;
    uint16_t duty;
    msg_t ret;

    if (channel == 0) {
        reg = LED0_OFF_L_REG;
    } else if (channel == 1) {
        reg = LED1_OFF_L_REG;
    } else {
        return 0;
    }

    // Write register pointer to LEDn_OFF_L (using read address 0x71 for expected transactions)
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, PCA9685_READ_ADDR, &reg, 1, NULL, 0, TIME_MS2I(100));
    if (ret != MSG_OK) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return 0;
    }
    // Read 4 bytes (LEDn_OFF_L, LEDn_OFF_H, LEDn+1_ON_L, LEDn+1_ON_H) using read address
    ret = i2cMasterReceiveTimeout((I2CDriver *)dev->bus_handle, PCA9685_READ_ADDR, buf, 4, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    if (ret != MSG_OK) {
        return 0;
    }

    off_l = buf[0];
    off_h = buf[1];
    duty = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    *out = duty;
    return duty;
}