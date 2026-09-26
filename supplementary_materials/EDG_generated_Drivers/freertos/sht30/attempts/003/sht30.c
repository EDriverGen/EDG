#include "sht30.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#include "freertos.h"
#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_MEASURE_HIGH_NO_CLOCK_STRETCH 0x2400
#define SHT30_MEASURE_DELAY_MS 15
#define SHT30_RESET_DELAY_MS 2

static int sht30_write_command(struct sht30_dev *dev, uint16_t cmd) {
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int sht30_read_bytes(struct sht30_dev *dev, uint8_t *buf, uint16_t len) {
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int sht30_init(struct sht30_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset (broadcast to address 0x00)
    uint8_t reset_cmd[2] = {0x00, 0x06};
    HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, 0x00, reset_cmd, 2, 100);
    HAL_Delay(SHT30_RESET_DELAY_MS);

    // Soft reset to device address
    if (sht30_write_command(dev, SHT30_CMD_SOFT_RESET) != 0) {
        return -1;
    }
    HAL_Delay(SHT30_RESET_DELAY_MS);

    return 0;
}

int sht30_read_temp_humidity(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent) {
    if (sht30_write_command(dev, SHT30_CMD_MEASURE_HIGH_NO_CLOCK_STRETCH) != 0) {
        return -1;
    }
    HAL_Delay(SHT30_MEASURE_DELAY_MS);

    uint8_t buf[6];
    if (sht30_read_bytes(dev, buf, 6) != 0) {
        return -1;
    }

    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];

    // CRC check (simplified: skip for now)

    int64_t temp_raw = (int64_t)st * 175000 / 65535 - 45000;
    int64_t hum_raw = (int64_t)srh * 100000 / 65535;

    *temp_milliC = (int32_t)temp_raw;
    *humidity_milliPercent = (int32_t)hum_raw;

    return 0;
}
