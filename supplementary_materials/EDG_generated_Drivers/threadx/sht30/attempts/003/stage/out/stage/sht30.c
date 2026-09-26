#include "sht30.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "threadx.h"
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

int sht30_init(struct sht30_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset to 0x00
    uint8_t cmd[] = {0x00, 0x06};
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, 0x00, cmd, 2, 100) != HAL_OK)
        return -1;
    HAL_Delay(2);

    // Soft reset to device address
    uint8_t cmd2[] = {0x30, 0xA2};
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, cmd2, 2, 100) != HAL_OK)
        return -1;
    HAL_Delay(2);

    return 0;
}

int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent) {
    uint8_t cmd[] = {0x24, 0x00};
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, cmd, 2, 100) != HAL_OK)
        return -1;

    HAL_Delay(15);

    uint8_t buf[6];
    if (HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, buf, 6, 100) != HAL_OK)
        return -1;

    // Verify CRC for temperature bytes
    if (crc8(buf, 2) != buf[2])
        return -1;
    // Verify CRC for humidity bytes
    if (crc8(buf + 3, 2) != buf[5])
        return -1;

    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];

    int64_t temp_raw = (int64_t)st * 175000 / 65535 - 45000;
    int64_t hum_raw = (int64_t)srh * 100000 / 65535;

    *temp_milliC = (int32_t)temp_raw;
    *humidity_milliPercent = (int32_t)hum_raw;

    return 0;
}
