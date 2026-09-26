#include "bme280.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define BME280_ADDR 0x76
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_STATUS 0xF3
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_HUM_MSB 0xFD
#define BME280_REG_CALIB 0x88
#define BME280_REG_CALIB_HUM 0xE1

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len) {
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

static int i2c_write(int fd, uint8_t addr, uint8_t reg, uint8_t data) {
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t buf[2] = {reg, data};
    int ret;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int bme280_init(struct bme280_dev *dev, const char *bus_handle) {
    int fd;
    uint8_t id;
    int ret;

    fd = open(bus_handle, O_RDWR);
    if (fd < 0) {
        return -EIO;
    }
    dev->fd = fd;
    dev->addr = BME280_ADDR;

    // Read chip ID
    ret = i2c_write_then_read(dev->fd, dev->addr, BME280_REG_ID, &id, 1);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }
    if (id != 0x60) {
        close(dev->fd);
        return -ENODEV;
    }

    // Soft reset
    ret = i2c_write(dev->fd, dev->addr, BME280_REG_RESET, 0xB6);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }
    usleep(2000); // 2 ms startup time

    // Write config register (filter, standby)
    ret = i2c_write(dev->fd, dev->addr, BME280_REG_CONFIG, 0x00);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    // Write ctrl_hum (oversampling x1)
    ret = i2c_write(dev->fd, dev->addr, BME280_REG_CTRL_HUM, 0x01);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    // Write ctrl_meas (oversampling x1, normal mode)
    ret = i2c_write(dev->fd, dev->addr, BME280_REG_CTRL_MEAS, 0x27);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw) {
    uint8_t buf[8];
    int ret;

    // Burst read from 0xF7, 8 bytes
    ret = i2c_write_then_read(dev->fd, dev->addr, BME280_REG_PRESS_MSB, buf, 8);
    if (ret < 0) {
        return ret;
    }

    // Pressure: 20-bit, big-endian, right-shift 4
    uint32_t press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    *pressure_raw = (int32_t)press;

    // Temperature: 20-bit, big-endian, right-shift 4
    uint32_t temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    *temp_raw = (int32_t)temp;

    // Humidity: 16-bit, big-endian
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];
    *humidity_raw = (int32_t)hum;

    return 0;
}
