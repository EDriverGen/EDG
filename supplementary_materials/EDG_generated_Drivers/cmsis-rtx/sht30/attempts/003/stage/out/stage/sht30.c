#include "sht30.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "cmsis_rtx.h"
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

static int i2c_write(struct sht30_dev *dev, uint16_t cmd) {
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     buf, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int i2c_write_raw(struct sht30_dev *dev, const uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     (uint8_t *)data, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int i2c_read(struct sht30_dev *dev, uint8_t *buf, uint16_t len) {
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle,
                                                    (uint16_t)(dev->i2c_addr << 1),
                                                    buf, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int sht30_init(struct sht30_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset (broadcast to address 0x00)
    uint8_t reset_cmd[2] = {0x00, 0x06};
    HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, 0x00, reset_cmd, 2, 100);
    HAL_Delay(2);

    // Soft reset
    if (i2c_write(dev, SHT30_CMD_SOFT_RESET) != 0)
        return -1;
    HAL_Delay(2);

    return 0;
}

int sht30_read_temp_humidity(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent) {
    // Send single shot measurement command
    if (i2c_write(dev, SHT30_CMD_SINGLE_SHOT_HIGH) != 0)
        return -1;

    HAL_Delay(15);

    // Read 6 bytes
    uint8_t buf[6];
    if (i2c_read(dev, buf, 6) != 0)
        return -1;

    // Verify CRC for temperature (bytes 0-1, crc at byte 2)
    if (crc8(buf, 2) != buf[2])
        return -1;
    // Verify CRC for humidity (bytes 3-4, crc at byte 5)
    if (crc8(buf + 3, 2) != buf[5])
        return -1;

    uint16_t st_raw = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh_raw = ((uint16_t)buf[3] << 8) | buf[4];

    // temperature: ((ST * 175000) // 65535) - 45000
    int64_t temp64 = (int64_t)st_raw * 175000LL;
    temp64 = temp64 / 65535LL;
    *temp_milliC = (int32_t)(temp64 - 45000LL);

    // humidity: ((SRH * 100000) // 65535)
    int64_t hum64 = (int64_t)srh_raw * 100000LL;
    hum64 = hum64 / 65535LL;
    *humidity_milliPercent = (int32_t)hum64;

    return 0;
}
