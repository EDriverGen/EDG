#include "sht30.h"
#include <stdint.h>
#include <string.h>
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"

#include "openharmony_liteosm.h"
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_SINGLE_SHOT_HIGH 0x2400
#define SHT30_READ_LEN 6

static uint8_t crc8(const uint8_t *data, size_t len) {
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

static int sht30_write_cmd(struct sht30_dev *dev, uint16_t cmd) {
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = 2;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

static int sht30_write_cmd_general_call(struct sht30_dev *dev, uint16_t cmd) {
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    struct I2cMsg msg;
    msg.addr = 0x00;
    msg.buf = buf;
    msg.len = 2;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

static int sht30_read_data(struct sht30_dev *dev, uint8_t *buf, size_t len) {
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = len;
    msg.flags = I2C_FLAG_READ;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

int sht30_init(struct sht30_dev *dev, DevHandle bus_handle) {
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset
    if (sht30_write_cmd_general_call(dev, SHT30_CMD_GENERAL_CALL_RESET) != 0) {
        return -1;
    }
    OsalMSleep(2); // wait 1.5ms, round up to 2ms

    // Soft reset
    if (sht30_write_cmd(dev, SHT30_CMD_SOFT_RESET) != 0) {
        return -1;
    }
    OsalMSleep(2); // wait 1.5ms, round up to 2ms

    return 0;
}

int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent) {
    if (dev == NULL || temp_milliC == NULL || humidity_milliPercent == NULL) {
        return -1;
    }

    // Send single shot measurement command (high repeatability, no clock stretching)
    if (sht30_write_cmd(dev, SHT30_CMD_SINGLE_SHOT_HIGH) != 0) {
        return -1;
    }

    // Wait for measurement (max 15ms)
    OsalMSleep(15);

    // Read 6 bytes: temp MSB, temp LSB, CRC, humidity MSB, humidity LSB, CRC
    uint8_t buf[SHT30_READ_LEN];
    if (sht30_read_data(dev, buf, SHT30_READ_LEN) != 0) {
        return -1;
    }

    // Verify CRC for temperature
    uint8_t crc_temp = crc8(buf, 2);
    if (crc_temp != buf[2]) {
        return -1; // CRC error
    }

    // Verify CRC for humidity
    uint8_t crc_hum = crc8(buf + 3, 2);
    if (crc_hum != buf[5]) {
        return -1; // CRC error
    }

    // Decode raw values (big-endian)
    uint16_t raw_temp = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_hum = ((uint16_t)buf[3] << 8) | buf[4];

    // Convert to milli units using integer approximation
    // temp: ((ST * 175000) / 65535) - 45000
    int64_t temp_val = ((int64_t)raw_temp * 175000) / 65535 - 45000;
    *temp_milliC = (int32_t)temp_val;

    // humidity: ((SRH * 100000) / 65535)
    int64_t hum_val = ((int64_t)raw_hum * 100000) / 65535;
    *humidity_milliPercent = (int32_t)hum_val;

    return 0;
}
