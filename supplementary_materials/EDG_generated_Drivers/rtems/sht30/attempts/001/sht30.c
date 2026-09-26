#include "sht30.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define SHT30_I2C_ADDR 0x44

#define CMD_SOFT_RESET 0x30A2
#define CMD_GENERAL_CALL_RESET 0x0006
#define CMD_SINGLE_SHOT_HIGH 0x2400

static int sht30_write_command(struct sht30_device *dev, uint16_t cmd)
{
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    struct i2c_msg msg;
    msg.addr = dev->i2c_addr;
    msg.flags = 0;
    msg.len = 2;
    msg.buf = buf;
    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;
    if (ioctl(dev->fd, I2C_RDWR, &rdwr) < 0) {
        return -errno;
    }
    return 0;
}

static int sht30_read_data(struct sht30_device *dev, uint8_t *buf, size_t len)
{
    struct i2c_msg msg;
    msg.addr = dev->i2c_addr;
    msg.flags = I2C_M_RD;
    msg.len = len;
    msg.buf = buf;
    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;
    if (ioctl(dev->fd, I2C_RDWR, &rdwr) < 0) {
        return -errno;
    }
    return 0;
}

static uint8_t sht30_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

int sht30_init(struct sht30_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    dev->fd = open(path, O_RDWR);
    if (dev->fd < 0) {
        return -errno;
    }
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset
    int ret = sht30_write_command(dev, CMD_GENERAL_CALL_RESET);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }
    usleep(1500); // 1.5 ms

    // Soft reset
    ret = sht30_write_command(dev, CMD_SOFT_RESET);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }
    usleep(1500); // 1.5 ms

    return 0;
}

int sht30_read_measurement(struct sht30_device *dev, int32_t *temp_milliC, int32_t *humidity_milliPct)
{
    // Send single shot measurement command
    int ret = sht30_write_command(dev, CMD_SINGLE_SHOT_HIGH);
    if (ret < 0) {
        return ret;
    }
    usleep(15000); // 15 ms

    // Read 6 bytes: temp MSB, temp LSB, CRC, humidity MSB, humidity LSB, CRC
    uint8_t buf[6];
    ret = sht30_read_data(dev, buf, 6);
    if (ret < 0) {
        return ret;
    }

    // Verify CRC for temperature
    uint8_t crc_temp = sht30_crc8(buf, 2);
    if (crc_temp != buf[2]) {
        return -EIO;
    }
    // Verify CRC for humidity
    uint8_t crc_hum = sht30_crc8(buf + 3, 2);
    if (crc_hum != buf[5]) {
        return -EIO;
    }

    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];

    // Temperature conversion: ((ST * 175000) // 65535) - 45000
    int64_t temp_raw = (int64_t)st * 175000;
    *temp_milliC = (int32_t)(temp_raw / 65535) - 45000;

    // Humidity conversion: ((SRH * 100000) // 65535)
    int64_t hum_raw = (int64_t)srh * 100000;
    *humidity_milliPct = (int32_t)(hum_raw / 65535);

    return 0;
}
