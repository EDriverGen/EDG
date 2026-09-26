#include "pca9685_ref.h"

static int _i2c_write_read(DevHandle bus, uint16_t addr,
                           uint8_t *wdata, uint16_t wlen,
                           uint8_t *rdata, uint16_t rlen)
{
    struct I2cMsg msg[2];
    uint8_t nmsg = 0;

    if (wlen > 0) {
        msg[nmsg].addr  = addr;
        msg[nmsg].flags = 0;
        msg[nmsg].buf   = wdata;
        msg[nmsg].len   = wlen;
        nmsg++;
    }
    if (rlen > 0) {
        msg[nmsg].addr  = addr;
        msg[nmsg].flags = I2C_FLAG_READ;
        msg[nmsg].buf   = rdata;
        msg[nmsg].len   = rlen;
        nmsg++;
    }
    return I2cTransfer(bus, msg, nmsg) == nmsg ? 0 : -1;
}

static int _read_regs(struct pca9685_device *dev, uint8_t reg,
                      uint8_t *buf, uint16_t len)
{
    if (!dev || !buf) return -1;
    return _i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}

static int _write_reg(struct pca9685_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    if (!dev) return -1;
    return _i2c_write_read(dev->bus, dev->addr, buf, 2, NULL, 0);
}

int pca9685_init(struct pca9685_device *dev, DevHandle bus, uint16_t addr)
{
    if (!dev) return -1;
    dev->bus  = bus;
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
    return _i2c_write_read(dev->bus, dev->addr, buf, 5, NULL, 0);
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
