#include "pca9685.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <xtimer.h>

#include "riot.h"
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_ALL_LED_OFF_L 0xFC

int pca9685_init(pca9685_t *dev, i2c_t bus, uint16_t addr) {
    dev->bus = bus;
    dev->addr = addr;
    return 0;
}

int pca9685_read_pwm_channel(pca9685_t *dev, uint8_t channel, uint16_t *value) {
    uint16_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t cmd = (uint8_t)(reg & 0xFF);
    uint8_t buf[4];
    int ret;

    ret = i2c_write_bytes(dev->bus, dev->addr, &cmd, 1, 0);
    if (ret != 0) {
        return -EIO;
    }

    ret = i2c_read_bytes(dev->bus, dev->addr, buf, 4, 0);
    if (ret != 0) {
        return -EIO;
    }

    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *value = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    return 0;
}
