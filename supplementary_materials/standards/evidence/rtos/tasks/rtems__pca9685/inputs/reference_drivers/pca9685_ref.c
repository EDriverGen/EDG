#include "pca9685_ref.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dev/i2c/i2c.h>

static int _read_regs(struct pca9685_device *dev, uint8_t reg,
                      uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int fd, ret;

    if (!dev || !dev->bus_path || !buf) return -1;

    msgs[0].addr  = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len   = len;
    msgs[1].buf   = buf;

    rdwr.msgs  = msgs;
    rdwr.nmsgs = 2;

    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) return -1;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    close(fd);
    return ret >= 0 ? 0 : -1;
}

static int _write_reg(struct pca9685_device *dev, uint8_t reg, uint8_t val)
{
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t frame[2] = {reg, val};
    int fd, ret;

    if (!dev || !dev->bus_path) return -1;

    msgs[0].addr  = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
    msgs[0].buf   = frame;
    rdwr.msgs  = msgs;
    rdwr.nmsgs = 1;

    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) return -1;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    close(fd);
    return ret >= 0 ? 0 : -1;
}

int pca9685_init(struct pca9685_device *dev, const char *bus_path, uint8_t addr)
{
    if (!dev || !bus_path) return -1;
    dev->bus_path = bus_path;
    dev->addr     = addr;

    if (_write_reg(dev, PCA9685_REG_PRESCALE, 0x1E) != 0)
        return -1;
    if (_write_reg(dev, PCA9685_REG_MODE1,
                   PCA9685_MODE1_AI | PCA9685_MODE1_ALLCALL) != 0)
        return -1;

    /* delay 1ms for oscillator start-up */
    usleep(1000);
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
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t frame[5];
    int fd, ret;

    if (!dev || !dev->bus_path || channel > 15) return -1;

    frame[0] = (uint8_t)(PCA9685_REG_LED0_ON_L + channel * 4);
    frame[1] = (uint8_t)(on & 0xFF);
    frame[2] = (uint8_t)((on >> 8) & 0x0F);
    if (on & 0x1000) frame[2] |= PCA9685_LED_FULL_ON;
    frame[3] = (uint8_t)(off & 0xFF);
    frame[4] = (uint8_t)((off >> 8) & 0x0F);
    if (off & 0x1000) frame[4] |= PCA9685_LED_FULL_OFF;

    msgs[0].addr  = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len   = 5;
    msgs[0].buf   = frame;
    rdwr.msgs  = msgs;
    rdwr.nmsgs = 1;

    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) return -1;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    close(fd);
    return ret >= 0 ? 0 : -1;
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
