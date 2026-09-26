#include "sht30.h"
#include <assert.h>
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_SINGLE_SHOT_HIGH 0x2400
#define SHT30_READ_LEN 6

static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}

static int i2c_write(uint8_t i2c_num, uint8_t addr, const uint8_t *buf, size_t len) {
    struct hal_i2c_master_data pdata = {
        .address = addr,
        .len = len,
        .buffer = (uint8_t *)buf,
    };
    return hal_i2c_master_write(i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
}

static int i2c_read(uint8_t i2c_num, uint8_t addr, uint8_t *buf, size_t len) {
    struct hal_i2c_master_data pdata = {
        .address = addr,
        .len = len,
        .buffer = buf,
    };
    return hal_i2c_master_read(i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
}

int sht30_init(struct sht30_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset (broadcast to address 0x00)
    uint8_t reset_cmd[] = {0x06};
    int rc = i2c_write(dev->i2c_num, 0x00, reset_cmd, sizeof(reset_cmd));
    if (rc != 0) return rc;
    os_time_delay(2); // 1.5 ms, round up to 2 ticks

    // Soft reset to device address
    uint8_t soft_reset[] = {0x30, 0xA2};
    rc = i2c_write(dev->i2c_num, dev->i2c_addr, soft_reset, sizeof(soft_reset));
    if (rc != 0) return rc;
    os_time_delay(2); // 1.5 ms

    return 0;
}

int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_val, int32_t *hum_val) {
    // Send single shot measurement command (high repeatability, no clock stretching)
    uint8_t cmd[] = {0x24, 0x00};
    int rc = i2c_write(dev->i2c_num, dev->i2c_addr, cmd, sizeof(cmd));
    if (rc != 0) return rc;

    // Wait for measurement (max 15 ms)
    os_time_delay(15);

    // Read 6 bytes: temp MSB, temp LSB, temp CRC, hum MSB, hum LSB, hum CRC
    uint8_t buf[6];
    rc = i2c_read(dev->i2c_num, dev->i2c_addr, buf, sizeof(buf));
    if (rc != 0) return rc;

    // Verify CRC for temperature
    uint8_t expected_crc_temp = crc8(buf, 2);
    if (expected_crc_temp != buf[2]) return -1; // -EIO

    // Verify CRC for humidity
    uint8_t expected_crc_hum = crc8(buf + 3, 2);
    if (expected_crc_hum != buf[5]) return -1; // -EIO

    // Convert raw values
    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];

    *temp_val = ((int32_t)st * 175000) / 65535 - 45000;
    *hum_val = ((int32_t)srh * 100000) / 65535;

    return 0;
}
