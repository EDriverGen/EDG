#include "sht30.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "stm32f1xx_hal_i2c.h"
#define SHT30_I2C_ADDR 0x44
#define SHT30_GENERAL_CALL_ADDR 0x00

#define CMD_SOFT_RESET 0x30A2
#define CMD_GENERAL_CALL_RESET 0x0006
#define CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH 0x2400

#define CRC8_POLY 0x31
#define CRC8_INIT 0xFF

static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = CRC8_INIT;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLY;
            else
                crc <<= 1;
        }
    }
    return crc;
}

static int i2c_write(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t *data, uint16_t len) {
    if (HAL_I2C_Master_Transmit(hi2c, (uint16_t)(addr << 1), data, len, 100) != HAL_OK)
        return -1;
    return 0;
}

static int i2c_read(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t *buf, uint16_t len) {
    if (HAL_I2C_Master_Receive(hi2c, (uint16_t)(addr << 1), buf, len, 100) != HAL_OK)
        return -1;
    return 0;
}

int sht30_init(struct sht30_dev *dev, void *bus_handle) {
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    uint8_t cmd[2];

    // General call reset
    cmd[0] = 0x06;
    if (i2c_write(dev->bus_handle, SHT30_GENERAL_CALL_ADDR, cmd, 1) != 0)
        return -1;
    HAL_Delay(2); // wait 1.5 ms, use 2 ms for safety

    // Soft reset
    cmd[0] = (uint8_t)(CMD_SOFT_RESET >> 8);
    cmd[1] = (uint8_t)(CMD_SOFT_RESET & 0xFF);
    if (i2c_write(dev->bus_handle, dev->i2c_addr, cmd, 2) != 0)
        return -1;
    HAL_Delay(2); // wait 1.5 ms, use 2 ms for safety

    return 0;
}

int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_val, int32_t *hum_val) {
    uint8_t cmd[2];
    uint8_t buf[6];

    // Send single shot measurement command
    cmd[0] = (uint8_t)(CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH >> 8);
    cmd[1] = (uint8_t)(CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH & 0xFF);
    if (i2c_write(dev->bus_handle, dev->i2c_addr, cmd, 2) != 0)
        return -1;

    // Wait for measurement completion
    HAL_Delay(15);

    // Read 6 bytes: temp MSB, temp LSB, temp CRC, hum MSB, hum LSB, hum CRC
    if (i2c_read(dev->bus_handle, dev->i2c_addr, buf, 6) != 0)
        return -1;

    // Verify CRC for temperature
    uint8_t expected_crc = crc8(buf, 2);
    if (expected_crc != buf[2])
        return -1; // CRC error

    // Verify CRC for humidity
    expected_crc = crc8(buf + 3, 2);
    if (expected_crc != buf[5])
        return -1; // CRC error

    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];

    // Convert to milli-degC: ((ST * 175000) // 65535) - 45000
    int32_t temp_milli = (int32_t)(((int64_t)st * 175000) / 65535) - 45000;
    // Convert to milli-percent: ((SRH * 100000) // 65535)
    int32_t hum_milli = (int32_t)(((int64_t)srh * 100000) / 65535);

    *temp_val = temp_milli;
    *hum_val = hum_milli;

    return 0;
}
