#include "pca9685_ref.h"

static int _read_regs(struct pca9685_device *dev, uint8_t reg,
                      uint8_t *buf, uint16_t len)
{
    if (!dev || !dev->bus || !buf) return -1;
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(dev->addr << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK ? 0 : -1;
}

static int _write_reg(struct pca9685_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    if (!dev || !dev->bus) return -1;
    return HAL_I2C_Master_Transmit(dev->bus, (uint16_t)(dev->addr << 1),
                                   buf, 2, 100) == HAL_OK ? 0 : -1;
}

int pca9685_init(struct pca9685_device *dev, I2C_HandleTypeDef *bus, uint8_t addr)
{
    if (!dev || !bus) return -1;
    dev->bus = bus;
    dev->addr = addr;

    if (HAL_I2C_Init(bus) != HAL_OK)
        return -1;

    if (_write_reg(dev, PCA9685_REG_PRESCALE, 0x1E) != 0)
        return -1;
    if (_write_reg(dev, PCA9685_REG_MODE1,
                   PCA9685_MODE1_AI | PCA9685_MODE1_ALLCALL) != 0)
        return -1;
    HAL_Delay(1);
    return 0;
}

int pca9685_read_pwm(struct pca9685_device *dev, uint8_t channel,
                     uint16_t *on, uint16_t *off)
{
    uint8_t buf[4], reg;
    if (!dev || channel > 15 || !on || !off) return -1;
    reg = (uint8_t)(PCA9685_REG_LED0_ON_L + channel * 4);
    if (_read_regs(dev, reg, buf, 4) != 0) return -1;
    *on  = (uint16_t)((buf[1] & 0x0F) << 8) | buf[0];
    if (buf[1] & PCA9685_LED_FULL_ON) *on |= 0x1000;
    *off = (uint16_t)((buf[3] & 0x0F) << 8) | buf[2];
    if (buf[3] & PCA9685_LED_FULL_OFF) *off |= 0x1000;
    return 0;
}

int pca9685_set_pwm(struct pca9685_device *dev, uint8_t channel,
                    uint16_t on, uint16_t off)
{
    uint8_t buf[5];
    if (!dev || channel > 15) return -1;
    buf[0] = (uint8_t)(PCA9685_REG_LED0_ON_L + channel * 4);
    buf[1] = (uint8_t)(on & 0xFF);
    buf[2] = (uint8_t)((on >> 8) & 0x0F);
    if (on & 0x1000) buf[2] |= PCA9685_LED_FULL_ON;
    buf[3] = (uint8_t)(off & 0xFF);
    buf[4] = (uint8_t)((off >> 8) & 0x0F);
    if (off & 0x1000) buf[4] |= PCA9685_LED_FULL_OFF;
    return HAL_I2C_Master_Transmit(dev->bus, (uint16_t)(dev->addr << 1),
                                   buf, 5, 100) == HAL_OK ? 0 : -1;
}

static int _read_ch(struct pca9685_device *dev, uint8_t ch) {
    uint16_t on, off;
    if (pca9685_read_pwm(dev, ch, &on, &off) != 0) return -1;
    if (on & 0x1000) return 4096;
    if (off & 0x1000) return 0;
    return (int)(off & 0x0FFF);
}

int pca9685_read_led0(struct pca9685_device *dev)  { return _read_ch(dev, 0); }
int pca9685_read_led1(struct pca9685_device *dev)  { return _read_ch(dev, 1); }
int pca9685_read_led2(struct pca9685_device *dev)  { return _read_ch(dev, 2); }
int pca9685_read_led3(struct pca9685_device *dev)  { return _read_ch(dev, 3); }
int pca9685_read_led4(struct pca9685_device *dev)  { return _read_ch(dev, 4); }
int pca9685_read_led5(struct pca9685_device *dev)  { return _read_ch(dev, 5); }
int pca9685_read_led6(struct pca9685_device *dev)  { return _read_ch(dev, 6); }
int pca9685_read_led7(struct pca9685_device *dev)  { return _read_ch(dev, 7); }
int pca9685_read_led8(struct pca9685_device *dev)  { return _read_ch(dev, 8); }
int pca9685_read_led9(struct pca9685_device *dev)  { return _read_ch(dev, 9); }
int pca9685_read_led10(struct pca9685_device *dev) { return _read_ch(dev, 10); }
int pca9685_read_led11(struct pca9685_device *dev) { return _read_ch(dev, 11); }
int pca9685_read_led12(struct pca9685_device *dev) { return _read_ch(dev, 12); }
int pca9685_read_led13(struct pca9685_device *dev) { return _read_ch(dev, 13); }
int pca9685_read_led14(struct pca9685_device *dev) { return _read_ch(dev, 14); }
int pca9685_read_led15(struct pca9685_device *dev) { return _read_ch(dev, 15); }
