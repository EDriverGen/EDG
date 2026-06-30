/*
 * SPDX-License-Identifier: MIT
 *
 * PCA9685 16-Channel 12-Bit PWM Controller Driver for ThreadX
 */
#ifndef __PCA9685_REF_H
#define __PCA9685_REF_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCA9685_I2C_ADDR         0x40
#define PCA9685_REG_MODE1        0x00
#define PCA9685_REG_MODE2        0x01
#define PCA9685_REG_PRESCALE     0xFE
#define PCA9685_REG_LED0_ON_L    0x06

/* MODE1 bits */
#define PCA9685_MODE1_RESTART    (1 << 7)
#define PCA9685_MODE1_EXTCLK     (1 << 6)
#define PCA9685_MODE1_AI         (1 << 5)
#define PCA9685_MODE1_SLEEP      (1 << 4)
#define PCA9685_MODE1_ALLCALL    (1 << 0)

/* LEDn_ON_H / LEDn_OFF_H bit 4 special functions */
#define PCA9685_LED_FULL_ON      (1 << 4)
#define PCA9685_LED_FULL_OFF     (1 << 4)

struct pca9685_i2c_ops {
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct pca9685_device {
    void *bus_context;
    const struct pca9685_i2c_ops *ops;
    uint16_t addr;
};

int pca9685_init(struct pca9685_device *dev, void *bus_context,
                 const struct pca9685_i2c_ops *ops, uint16_t addr);
int pca9685_read_pwm(struct pca9685_device *dev, uint8_t channel,
                     uint16_t *on, uint16_t *off);
int pca9685_set_pwm(struct pca9685_device *dev, uint8_t channel,
                    uint16_t on, uint16_t off);

/* Per-channel read helpers: returns 12-bit OFF count,
   4096 if full ON, -1 on error */
int pca9685_read_led0(struct pca9685_device *dev);
int pca9685_read_led1(struct pca9685_device *dev);
int pca9685_read_led2(struct pca9685_device *dev);
int pca9685_read_led3(struct pca9685_device *dev);
int pca9685_read_led4(struct pca9685_device *dev);
int pca9685_read_led5(struct pca9685_device *dev);
int pca9685_read_led6(struct pca9685_device *dev);
int pca9685_read_led7(struct pca9685_device *dev);
int pca9685_read_led8(struct pca9685_device *dev);
int pca9685_read_led9(struct pca9685_device *dev);
int pca9685_read_led10(struct pca9685_device *dev);
int pca9685_read_led11(struct pca9685_device *dev);
int pca9685_read_led12(struct pca9685_device *dev);
int pca9685_read_led13(struct pca9685_device *dev);
int pca9685_read_led14(struct pca9685_device *dev);
int pca9685_read_led15(struct pca9685_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* __PCA9685_REF_H */
