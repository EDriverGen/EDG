#include "pca9685.h"
#include "hal.h"
#include "hal_i2c.h"
#include <string.h>
#include <stdint.h>

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_READ_ADDR 0x71
#define MODE1_REG 0x00
#define MODE2_REG 0x01
#define LED0_OFF_L_REG 0x08
#define LED0_OFF_H_REG 0x09
#define LED1_OFF_L_REG 0x0A
#define LED1_OFF_H_REG 0x0B

static msg_t i2c_write_then_read(I2CDriver *i2cp, i2caddr_t addr, const uint8_t *txbuf, size_t txbytes, uint8_t *rxbuf, size_t rxbytes, sysinterval_t timeout) {
    return i2cMasterTransmitTimeout(i2cp, addr, txbuf, txbytes, rxbuf, rxbytes, timeout);
}

void pca9685_init(struct pca9685_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
}

int pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *out) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t reg;
    uint8_t rxbuf[4];
    msg_t status;
    sysinterval_t timeout = TIME_MS2I(100);

    if (channel == 0) {
        reg = LED0_OFF_L_REG;
    } else if (channel == 1) {
        reg = LED1_OFF_L_REG;
    } else {
        return -1;
    }

    i2cAcquireBus(i2cp);
    status = i2c_write_then_read(i2cp, PCA9685_READ_ADDR, &reg, 1, rxbuf, 4, timeout);
    i2cReleaseBus(i2cp);

    if (status != MSG_OK) {
        return -1;
    }

    uint8_t off_l = rxbuf[0];
    uint8_t off_h = rxbuf[1];
    uint16_t duty = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    *out = duty;

    return 0;
}