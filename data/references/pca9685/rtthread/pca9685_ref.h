#ifndef __PCA9685_REF_H
#define __PCA9685_REF_H

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

#define PCA9685_I2C_ADDR    0x40
#define PCA9685_REG_MODE1   0x00
#define PCA9685_REG_MODE2   0x01
#define PCA9685_REG_PRESCALE 0xFE
#define PCA9685_REG_LED0_ON_L 0x06

/* MODE1 bits */
#define PCA9685_MODE1_RESTART  (1 << 7)
#define PCA9685_MODE1_EXTCLK   (1 << 6)
#define PCA9685_MODE1_AI       (1 << 5)
#define PCA9685_MODE1_SLEEP    (1 << 4)
#define PCA9685_MODE1_ALLCALL  (1 << 0)

/* LEDn_ON_H / LEDn_OFF_H bit 4 special functions */
#define PCA9685_LED_FULL_ON   (1 << 4)
#define PCA9685_LED_FULL_OFF  (1 << 4)

struct pca9685_device {
    struct rt_i2c_bus_device *bus;
    uint16_t addr;
};

rt_err_t pca9685_init(struct pca9685_device *dev,
                      struct rt_i2c_bus_device *bus, uint16_t addr);
rt_err_t pca9685_read_pwm(struct pca9685_device *dev, uint8_t channel,
                          uint16_t *on, uint16_t *off);
rt_err_t pca9685_set_pwm(struct pca9685_device *dev, uint8_t channel,
                         uint16_t on, uint16_t off);

/* Per-channel read helpers for eval: returns 12-bit OFF count,
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

#endif
