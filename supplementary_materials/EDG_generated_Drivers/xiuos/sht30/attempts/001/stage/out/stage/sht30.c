#include "sht30.h"
#include "transform.h"
#include "bus.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bus_i2c.h"
#include "dev_i2c.h"
#include "bus_pin.h"
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH 0x2400
#define SHT30_READ_LEN 6

static int sht30_write_command(struct sht30_device *dev, uint16_t cmd) {
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    int ret = PrivWrite(dev->fd, buf, 2);
    if (ret < 0) return ret;
    return 0;
}

static int sht30_read_data(struct sht30_device *dev, uint8_t *buf, size_t len) {
    int ret = PrivRead(dev->fd, buf, len);
    if (ret < 0) return ret;
    return 0;
}

static uint8_t sht30_crc8(const uint8_t *data, size_t len) {
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

int sht30_init(struct sht30_device *dev, struct I2cBus *bus_handle) {
    if (!dev || !bus_handle) return -EINVAL;
    dev->bus = bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // Open I2C device
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return dev->fd;

    // Set slave address
    uint16_t i2c_addr = dev->i2c_addr;
    struct PrivIoctlCfg ioctl_cfg;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    int ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        return ret;
    }

    // General call reset (broadcast to address 0x00)
    uint8_t gen_call_buf[2] = {0x00, 0x06};
    ret = PrivWrite(dev->fd, gen_call_buf, 2);
    if (ret < 0) {
        PrivClose(dev->fd);
        return ret;
    }
    PrivTaskDelay(2); // wait 1.5 ms, round up to 2 ms

    // Soft reset
    ret = sht30_write_command(dev, SHT30_CMD_SOFT_RESET);
    if (ret < 0) {
        PrivClose(dev->fd);
        return ret;
    }
    PrivTaskDelay(2); // wait 1.5 ms, round up to 2 ms

    return 0;
}

int sht30_read_temp_humidity(struct sht30_device *dev, int32_t *temp_milliC, int32_t *humidity_milliPct) {
    if (!dev || !temp_milliC || !humidity_milliPct) return -EINVAL;

    // Send single shot measurement command
    int ret = sht30_write_command(dev, SHT30_CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH);
    if (ret < 0) return ret;

    // Wait for measurement (max 15 ms)
    PrivTaskDelay(15);

    // Read 6 bytes: temp MSB, temp LSB, temp CRC, humidity MSB, humidity LSB, humidity CRC
    uint8_t buf[SHT30_READ_LEN];
    ret = sht30_read_data(dev, buf, SHT30_READ_LEN);
    if (ret < 0) return ret;

    // Verify CRC for temperature bytes
    uint8_t crc_temp = sht30_crc8(buf, 2);
    if (crc_temp != buf[2]) return -EIO;

    // Verify CRC for humidity bytes
    uint8_t crc_hum = sht30_crc8(buf + 3, 2);
    if (crc_hum != buf[5]) return -EIO;

    // Decode raw values (big-endian)
    uint16_t raw_temp = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_hum = ((uint16_t)buf[3] << 8) | buf[4];

    // Convert to milli units using integer approximation
    // temp: ((raw_temp * 175000) / 65535) - 45000
    int64_t temp_val = ((int64_t)raw_temp * 175000) / 65535 - 45000;
    *temp_milliC = (int32_t)temp_val;

    // humidity: (raw_hum * 100000) / 65535
    int64_t hum_val = ((int64_t)raw_hum * 100000) / 65535;
    *humidity_milliPct = (int32_t)hum_val;

    return 0;
}
