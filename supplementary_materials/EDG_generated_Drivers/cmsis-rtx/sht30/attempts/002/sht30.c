#include "sht30.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#include "cmsis_rtx.h"
#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_MEASURE_HIGH 0x2400
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

static int i2c_write(struct sht30_dev *dev, uint16_t addr, uint8_t *data, uint16_t size) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, addr << 1, data, size, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int i2c_read(struct sht30_dev *dev, uint16_t addr, uint8_t *data, uint16_t size) {
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(hi2c, addr << 1, data, size, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int sht30_init(struct sht30_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset to all devices on bus (address 0x00)
    uint8_t reset_cmd[] = {0x00, 0x06};
    if (i2c_write(dev, 0x00, reset_cmd, 2) != 0)
        return -1;
    HAL_Delay(2);

    // Soft reset to this device
    uint8_t soft_reset_cmd[] = {0x30, 0xA2};
    if (i2c_write(dev, dev->i2c_addr, soft_reset_cmd, 2) != 0)
        return -1;
    HAL_Delay(2);

    return 0;
}

int sht30_read_temp_humidity(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent) {
    uint8_t cmd[] = {0x24, 0x00};
    if (i2c_write(dev, dev->i2c_addr, cmd, 2) != 0)
        return -1;

    HAL_Delay(15);

    uint8_t buf[SHT30_READ_LEN];
    if (i2c_read(dev, dev->i2c_addr, buf, SHT30_READ_LEN) != 0)
        return -1;

    // Verify CRC for temperature bytes
    if (crc8(buf, 2) != buf[2])
        return -1;
    // Verify CRC for humidity bytes
    if (crc8(buf + 3, 2) != buf[5])
        return -1;

    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];

    *temp_milliC = ((int32_t)((uint32_t)st * 175000 / 65535)) - 45000;
    *humidity_milliPercent = (int32_t)((uint32_t)srh * 100000 / 65535);

    return 0;
}
