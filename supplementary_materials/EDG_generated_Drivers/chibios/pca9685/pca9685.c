#include "pca9685.h"
#include "hal.h"
#include <string.h>

#include "hal_i2c.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_READ_ADDR 0x71
#define MODE1 0x00
#define MODE2 0x01
#define LED0_OFF_L 0x08

static msg_t i2c_write_then_read(struct pca9685_device *dev, uint8_t reg, uint8_t *rxbuf, size_t rxbytes) {
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, PCA9685_READ_ADDR, &reg, 1, rxbuf, rxbytes, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return ret;
}

void pca9685_init(struct pca9685_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCA9685_I2C_ADDR;
}

uint16_t pca9685_read_pwm_channel(struct pca9685_device *dev, uint8_t channel, uint16_t *out) {
    (void)channel;
    uint8_t buf[4];
    uint8_t reg = LED0_OFF_L;
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, PCA9685_READ_ADDR, &reg, 1, buf, 4, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    if (ret != MSG_OK) {
        *out = 0;
        return 1;
    }
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    uint16_t duty = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    *out = duty;
    return 0;
}
