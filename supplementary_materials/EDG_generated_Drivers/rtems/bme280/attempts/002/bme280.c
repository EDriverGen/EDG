#include "bme280.h"
#include "rtems.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#define BME280_I2C_ADDR 0x76

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int i2c_write(int fd, uint8_t addr, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = len;
    msgs[0].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int bme280_init(struct bme280_dev *dev, const char *bus_handle)
{
    int fd;
    uint8_t chip_id;
    uint8_t reset_cmd = 0xB6;
    uint8_t config_val = 0x00;
    int ret;

    fd = open(bus_handle, O_RDWR);
    if (fd < 0) {
        return -EIO;
    }
    dev->fd = fd;
    dev->i2c_addr = BME280_I2C_ADDR;

    /* Read chip ID */
    ret = i2c_write_then_read(dev->fd, dev->i2c_addr, 0xD0, &chip_id, 1);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }
    if (chip_id != 0x60) {
        close(dev->fd);
        return -ENODEV;
    }

    /* Soft reset */
    ret = i2c_write(dev->fd, dev->i2c_addr, &reset_cmd, 1);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    /* Write config register (filter off, standby 0.5ms) */
    ret = i2c_write(dev->fd, dev->i2c_addr, &config_val, 1);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    uint8_t buf[8];
    uint32_t temp, press;
    uint16_t hum;
    int ret;

    /* Burst read from 0xF7, 8 bytes */
    ret = i2c_write_then_read(dev->fd, dev->i2c_addr, 0xF7, buf, 8);
    if (ret < 0) {
        return ret;
    }

    /* Pressure: 20-bit, big-endian, right-shift 4 */
    press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    /* Temperature: 20-bit, big-endian, right-shift 4 */
    temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    /* Humidity: 16-bit, big-endian */
    hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}
