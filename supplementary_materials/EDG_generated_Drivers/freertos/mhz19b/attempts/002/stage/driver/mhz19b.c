#include "mhz19b.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define CMD_READ_CO2 0x86
#define RESPONSE_LENGTH 9
#define START_BYTE 0xFF
#define CMD_BYTE_INDEX 1
#define HIGH_BYTE_INDEX 2
#define LOW_BYTE_INDEX 3
#define CHECKSUM_INDEX 8

static uint8_t calculate_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (~sum) + 1;
}

int mhz19b_init(struct mhz19b_dev *dev, void *bus_handle) {
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw) {
    if (dev == NULL || dev->bus_handle == NULL || raw == NULL) return -1;

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)dev->bus_handle;

    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[RESPONSE_LENGTH];

    // Send command
    if (HAL_UART_Transmit(huart, cmd, 9, 1000) != HAL_OK) {
        return -1;
    }

    // Wait for sensor to process (at least 10ms)
    HAL_Delay(10);

    // Read response
    if (HAL_UART_Receive(huart, resp, RESPONSE_LENGTH, 1000) != HAL_OK) {
        return -1;
    }

    // Validate start byte
    if (resp[0] != START_BYTE) {
        return -1;
    }

    // Validate command byte
    if (resp[CMD_BYTE_INDEX] != CMD_READ_CO2) {
        return -1;
    }

    // Validate checksum
    uint8_t expected_checksum = calculate_checksum(resp, RESPONSE_LENGTH - 1);
    if (resp[CHECKSUM_INDEX] != expected_checksum) {
        return -1;
    }

    // Extract CO2 value (big-endian)
    uint16_t co2 = ((uint16_t)resp[HIGH_BYTE_INDEX] << 8) | resp[LOW_BYTE_INDEX];
    *raw = (int32_t)co2;

    return 0;
}