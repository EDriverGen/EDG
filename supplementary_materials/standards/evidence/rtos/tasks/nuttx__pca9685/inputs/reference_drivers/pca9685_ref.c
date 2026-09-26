/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCA9685 16-Channel 12-Bit PWM Controller Driver for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-19     Lin          NuttX reference driver
 */
#include "pca9685_ref.h"
#include <errno.h>

static int _read_regs(FAR struct pca9685_device *dev, uint8_t reg,
                      FAR uint8_t *buf, int len)
{
    if (!dev || !dev->i2c || !buf) return -EINVAL;
    return i2c_writeread(dev->i2c, &dev->config, &reg, 1, buf, len);
}

static int _write_reg(FAR struct pca9685_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t frame[2];

    if (!dev || !dev->i2c) return -EINVAL;

    frame[0] = reg;
    frame[1] = val;
    return i2c_write(dev->i2c, &dev->config, frame, 2);
}

int pca9685_init(FAR struct pca9685_device *dev,
                 FAR struct i2c_master_s *i2c, uint8_t addr)
{
    if (!dev || !i2c) return -EINVAL;

    dev->i2c = i2c;
    dev->config.frequency = PCA9685_I2C_FREQ;
    dev->config.address   = addr;
    dev->config.addrlen   = 7;

    /* POR default MODE1=0x11 (SLEEP=1, ALLCALL=1).
     * PRE_SCALE only writable when SLEEP=1. */
    if (_write_reg(dev, PCA9685_REG_PRESCALE, 0x1E) < 0)
        return -EIO;

    /* Wake up: AI=1, SLEEP=0, ALLCALL=1 */
    if (_write_reg(dev, PCA9685_REG_MODE1,
                   PCA9685_MODE1_AI | PCA9685_MODE1_ALLCALL) < 0)
        return -EIO;

    /* Oscillator start-up: 500 us max per datasheet */
    usleep(500);

    return 0;
}

int pca9685_read_pwm(FAR struct pca9685_device *dev, uint8_t channel,
                     FAR uint16_t *on, FAR uint16_t *off)
{
    uint8_t buf[4];
    uint8_t reg;
    int ret;

    if (!dev || channel > 15 || !on || !off) return -EINVAL;

    reg = (uint8_t)(PCA9685_REG_LED0_ON_L + (channel * 4));
    ret = _read_regs(dev, reg, buf, 4);
    if (ret < 0) return ret;

    *on  = (uint16_t)((buf[1] & 0x0F) << 8) | buf[0];
    if (buf[1] & PCA9685_LED_FULL_ON)
        *on |= 0x1000;

    *off = (uint16_t)((buf[3] & 0x0F) << 8) | buf[2];
    if (buf[3] & PCA9685_LED_FULL_OFF)
        *off |= 0x1000;

    return 0;
}

int pca9685_set_pwm(FAR struct pca9685_device *dev, uint8_t channel,
                    uint16_t on, uint16_t off)
{
    uint8_t frame[5];

    if (!dev || channel > 15) return -EINVAL;

    frame[0] = (uint8_t)(PCA9685_REG_LED0_ON_L + (channel * 4));
    frame[1] = (uint8_t)(on & 0xFF);          /* LEDn_ON_L */
    frame[2] = (uint8_t)((on >> 8) & 0x0F);   /* LEDn_ON_H[3:0] */
    if (on & 0x1000) frame[2] |= PCA9685_LED_FULL_ON;
    frame[3] = (uint8_t)(off & 0xFF);         /* LEDn_OFF_L */
    frame[4] = (uint8_t)((off >> 8) & 0x0F);  /* LEDn_OFF_H[3:0] */
    if (off & 0x1000) frame[4] |= PCA9685_LED_FULL_OFF;

    return i2c_write(dev->i2c, &dev->config, frame, 5);
}

/* Per-channel readers */

static int _read_ch(FAR struct pca9685_device *dev, uint8_t ch) {
    uint16_t on, off;
    if (pca9685_read_pwm(dev, ch, &on, &off) != 0) return -1;
    if (on & 0x1000) return 4096;
    if (off & 0x1000) return 0;
    return (int)(off & 0x0FFF);
}

int pca9685_read_led0(FAR struct pca9685_device *dev)  { return _read_ch(dev, 0); }
int pca9685_read_led1(FAR struct pca9685_device *dev)  { return _read_ch(dev, 1); }
int pca9685_read_led2(FAR struct pca9685_device *dev)  { return _read_ch(dev, 2); }
int pca9685_read_led3(FAR struct pca9685_device *dev)  { return _read_ch(dev, 3); }
int pca9685_read_led4(FAR struct pca9685_device *dev)  { return _read_ch(dev, 4); }
int pca9685_read_led5(FAR struct pca9685_device *dev)  { return _read_ch(dev, 5); }
int pca9685_read_led6(FAR struct pca9685_device *dev)  { return _read_ch(dev, 6); }
int pca9685_read_led7(FAR struct pca9685_device *dev)  { return _read_ch(dev, 7); }
int pca9685_read_led8(FAR struct pca9685_device *dev)  { return _read_ch(dev, 8); }
int pca9685_read_led9(FAR struct pca9685_device *dev)  { return _read_ch(dev, 9); }
int pca9685_read_led10(FAR struct pca9685_device *dev) { return _read_ch(dev, 10); }
int pca9685_read_led11(FAR struct pca9685_device *dev) { return _read_ch(dev, 11); }
int pca9685_read_led12(FAR struct pca9685_device *dev) { return _read_ch(dev, 12); }
int pca9685_read_led13(FAR struct pca9685_device *dev) { return _read_ch(dev, 13); }
int pca9685_read_led14(FAR struct pca9685_device *dev) { return _read_ch(dev, 14); }
int pca9685_read_led15(FAR struct pca9685_device *dev) { return _read_ch(dev, 15); }
