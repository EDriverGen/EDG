#ifndef __PCA9685_REF_H
#define __PCA9685_REF_H

#include <transform.h>
#include <stdint.h>

#define PCA9685_I2C_ADDR         0x40
#define PCA9685_REG_MODE1        0x00
#define PCA9685_REG_PRESCALE     0xFE
#define PCA9685_REG_LED0_ON_L    0x06

#define PCA9685_MODE1_AI         (1 << 5)
#define PCA9685_MODE1_SLEEP      (1 << 4)
#define PCA9685_MODE1_ALLCALL    (1 << 0)
#define PCA9685_LED_FULL_ON      (1 << 4)
#define PCA9685_LED_FULL_OFF     (1 << 4)

struct pca9685_device {
    int      fd;
    uint16_t addr;
};

int pca9685_init(struct pca9685_device *dev, const char *i2c_path, uint16_t addr);
int pca9685_read_pwm(struct pca9685_device *dev, uint8_t channel,
                     uint16_t *on, uint16_t *off);
int pca9685_set_pwm(struct pca9685_device *dev, uint8_t channel,
                    uint16_t on, uint16_t off);

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
