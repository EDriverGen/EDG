/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCA9685 16-Channel 12-Bit PWM Controller Driver for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-19     Lin          NuttX reference driver
 */
#ifndef __PCA9685_REF_H
#define __PCA9685_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCA9685_I2C_ADDR         0x40
#define PCA9685_I2C_FREQ         100000
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

struct pca9685_device {
    FAR struct i2c_master_s *i2c;
    struct i2c_config_s config;
};

int pca9685_init(FAR struct pca9685_device *dev,
                 FAR struct i2c_master_s *i2c, uint8_t addr);
int pca9685_read_pwm(FAR struct pca9685_device *dev, uint8_t channel,
                     FAR uint16_t *on, FAR uint16_t *off);
int pca9685_set_pwm(FAR struct pca9685_device *dev, uint8_t channel,
                    uint16_t on, uint16_t off);

/* Per-channel read helpers: returns 12-bit OFF count,
   4096 if full ON, -1 on error */
int pca9685_read_led0(FAR struct pca9685_device *dev);
int pca9685_read_led1(FAR struct pca9685_device *dev);
int pca9685_read_led2(FAR struct pca9685_device *dev);
int pca9685_read_led3(FAR struct pca9685_device *dev);
int pca9685_read_led4(FAR struct pca9685_device *dev);
int pca9685_read_led5(FAR struct pca9685_device *dev);
int pca9685_read_led6(FAR struct pca9685_device *dev);
int pca9685_read_led7(FAR struct pca9685_device *dev);
int pca9685_read_led8(FAR struct pca9685_device *dev);
int pca9685_read_led9(FAR struct pca9685_device *dev);
int pca9685_read_led10(FAR struct pca9685_device *dev);
int pca9685_read_led11(FAR struct pca9685_device *dev);
int pca9685_read_led12(FAR struct pca9685_device *dev);
int pca9685_read_led13(FAR struct pca9685_device *dev);
int pca9685_read_led14(FAR struct pca9685_device *dev);
int pca9685_read_led15(FAR struct pca9685_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* __PCA9685_REF_H */
