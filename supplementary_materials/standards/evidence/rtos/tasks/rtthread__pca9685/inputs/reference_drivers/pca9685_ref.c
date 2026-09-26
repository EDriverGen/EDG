#include "pca9685_ref.h"

/* Internal: write register pointer then read n bytes */
static int _read_regs(struct pca9685_device *dev, uint8_t reg,
                      uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[2];
    msgs[0].addr  = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;
    msgs[1].addr  = dev->addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buf;
    msgs[1].len   = len;
    return rt_i2c_transfer(dev->bus, msgs, 2) == 2 ? RT_EOK : -RT_ERROR;
}

/* Internal: write register with 1 byte value */
static int _write_reg(struct pca9685_device *dev, uint8_t reg, uint8_t val)
{
    struct rt_i2c_msg msgs[1];
    uint8_t buf[2] = {reg, val};
    msgs[0].addr  = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buf;
    msgs[0].len   = 2;
    return rt_i2c_transfer(dev->bus, msgs, 1) == 1 ? RT_EOK : -RT_ERROR;
}

rt_err_t pca9685_init(struct pca9685_device *dev,
                      struct rt_i2c_bus_device *bus, uint16_t addr)
{
    if (!dev || !bus) return -RT_EINVAL;
    dev->bus  = bus;
    dev->addr = addr;

    /* POR default MODE1=0x11 (SLEEP=1, ALLCALL=1).
     * PRE_SCALE only writable when SLEEP=1. */
    if (_write_reg(dev, PCA9685_REG_PRESCALE, 0x1E) != RT_EOK)
        return -RT_ERROR;

    /* Wake up: AI=1, SLEEP=0, ALLCALL=1 */
    if (_write_reg(dev, PCA9685_REG_MODE1,
                   PCA9685_MODE1_AI | PCA9685_MODE1_ALLCALL) != RT_EOK)
        return -RT_ERROR;

    /* Oscillator start-up: 500 μs max per datasheet §7.3.1 */
    rt_thread_mdelay(1);

    return RT_EOK;
}

rt_err_t pca9685_read_pwm(struct pca9685_device *dev, uint8_t channel,
                          uint16_t *on, uint16_t *off)
{
    uint8_t buf[4];
    uint8_t reg = (uint8_t)(PCA9685_REG_LED0_ON_L + (channel * 4));

    if (!dev || channel > 15 || !on || !off) return -RT_EINVAL;
    if (_read_regs(dev, reg, buf, 4) != RT_EOK)
        return -RT_ERROR;

    *on  = (uint16_t)((buf[1] & 0x0F) << 8) | buf[0];
    if (buf[1] & PCA9685_LED_FULL_ON)
        *on |= 0x1000;  /* flag full-on in bit 12 */

    *off = (uint16_t)((buf[3] & 0x0F) << 8) | buf[2];
    if (buf[3] & PCA9685_LED_FULL_OFF)
        *off |= 0x1000;

    return RT_EOK;
}

rt_err_t pca9685_set_pwm(struct pca9685_device *dev, uint8_t channel,
                         uint16_t on, uint16_t off)
{
    uint8_t buf[5];
    uint8_t reg = (uint8_t)(PCA9685_REG_LED0_ON_L + (channel * 4));

    if (!dev || channel > 15) return -RT_EINVAL;

    buf[0] = reg;
    buf[1] = (uint8_t)(on & 0xFF);          /* LEDn_ON_L */
    buf[2] = (uint8_t)((on >> 8) & 0x0F);   /* LEDn_ON_H[3:0] */
    if (on & 0x1000) buf[2] |= PCA9685_LED_FULL_ON;
    buf[3] = (uint8_t)(off & 0xFF);         /* LEDn_OFF_L */
    buf[4] = (uint8_t)((off >> 8) & 0x0F);  /* LEDn_OFF_H[3:0] */
    if (off & 0x1000) buf[4] |= PCA9685_LED_FULL_OFF;

    struct rt_i2c_msg msgs[1];
    msgs[0].addr  = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buf;
    msgs[0].len   = 5;
    return rt_i2c_transfer(dev->bus, msgs, 1) == 1 ? RT_EOK : -RT_ERROR;
}

/* Per-channel readers */

static int _read_ch(struct pca9685_device *dev, uint8_t ch) {
    uint16_t on, off;
    if (pca9685_read_pwm(dev, ch, &on, &off) != RT_EOK) return -1;
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
