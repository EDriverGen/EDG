#include "pca9685.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <xtimer.h>

#include "riot.h"
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_MODE1_SLEEP (1 << 4)
#define PCA9685_MODE1_AI (1 << 5)

static int pca9685_write_reg(pca9685_t *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    int ret = i2c_write_bytes(dev->bus, dev->addr, buf, 2, 0);
    if (ret < 0) return -EIO;
    return 0;
}

static int pca9685_read_reg(pca9685_t *dev, uint8_t reg, uint8_t *val) {
    int ret = i2c_read_regs(dev->bus, dev->addr, reg, val, 1, 0);
    if (ret < 0) return -EIO;
    return 0;
}

int pca9685_init(pca9685_t *dev, i2c_t bus, uint8_t addr) {
    dev->bus = bus;
    dev->addr = addr;
    
    // Wake up from sleep: clear SLEEP bit in MODE1
    uint8_t mode1;
    int ret = pca9685_read_reg(dev, PCA9685_MODE1, &mode1);
    if (ret < 0) return ret;
    mode1 &= ~PCA9685_MODE1_SLEEP;
    ret = pca9685_write_reg(dev, PCA9685_MODE1, mode1);
    if (ret < 0) return ret;
    
    // Wait for oscillator startup
    xtimer_msleep(1); // 500 us minimum, use 1 ms for safety
    
    // Enable auto-increment for burst reads
    mode1 |= PCA9685_MODE1_AI;
    ret = pca9685_write_reg(dev, PCA9685_MODE1, mode1);
    if (ret < 0) return ret;
    
    return 0;
}

int pca9685_read_pwm_channel(pca9685_t *dev, uint8_t channel, uint16_t *value) {
    // Calculate register address for LEDn_OFF_L (0x08 + 4*channel)
    uint8_t reg = PCA9685_LED0_OFF_L + 4 * channel;
    uint8_t buf[4];
    
    // Write register pointer then read 4 bytes (OFF_L, OFF_H, next ON_L, next ON_H)
    int ret = i2c_read_regs(dev->bus, dev->addr, reg, buf, 4, 0);
    if (ret < 0) return -EIO;
    
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *value = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    return 0;
}
