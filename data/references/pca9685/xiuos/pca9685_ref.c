#include "pca9685_ref.h"

static int _write_then_read(struct pca9685_device *dev,
                            uint8_t *wbuf, uint16_t wlen,
                            uint8_t *rbuf, uint16_t rlen)
{
    if (PrivWrite(dev->fd, wbuf, wlen) != (int)wlen)
        return -1;
    if (rlen > 0 && PrivRead(dev->fd, rbuf, rlen) != (int)rlen)
        return -1;
    return 0;
}

static int _read_regs(struct pca9685_device *dev, uint8_t reg,
                      uint8_t *buf, uint16_t len)
{
    if (!dev || !buf) return -1;
    return _write_then_read(dev, &reg, 1, buf, len);
}

static int _write_reg(struct pca9685_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    if (!dev) return -1;
    return (PrivWrite(dev->fd, buf, 2) == 2) ? 0 : -1;
}

int pca9685_init(struct pca9685_device *dev, const char *i2c_path, uint16_t addr)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = addr;
    if (!dev || !i2c_path) return -1;

    dev->fd = PrivOpen(i2c_path, O_RDWR);
    if (dev->fd < 0) return -1;

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0) {
        PrivClose(dev->fd);
        return -1;
    }
    dev->addr = addr;

    if (_write_reg(dev, PCA9685_REG_PRESCALE, 0x1E) != 0)
        return -1;
    if (_write_reg(dev, PCA9685_REG_MODE1,
                   PCA9685_MODE1_AI | PCA9685_MODE1_ALLCALL) != 0)
        return -1;

    /* delay 1ms for oscillator start-up */
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
    return (PrivWrite(dev->fd, buf, 5) == 5) ? 0 : -1;
}

static int _read_ch(struct pca9685_device *dev, uint8_t ch)
{
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
