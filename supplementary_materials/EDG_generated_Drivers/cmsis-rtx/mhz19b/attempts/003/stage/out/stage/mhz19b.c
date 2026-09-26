#include "mhz19b.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9

static uint8_t mhz19b_calc_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw) {
    if (!dev || !raw) return -1;

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)dev->bus_handle;
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[MHZ19B_RESPONSE_LENGTH];

    // Send command
    if (HAL_UART_Transmit(huart, cmd, 9, 1000) != HAL_OK) {
        return -1;
    }

    // Wait for response (sensor needs some time)
    HAL_Delay(10);

    // Read response
    if (HAL_UART_Receive(huart, resp, MHZ19B_RESPONSE_LENGTH, 1000) != HAL_OK) {
        return -1;
    }

    // Validate start byte
    if (resp[0] != 0xFF) {
        return -1;
    }

    // Validate command byte
    if (resp[1] != 0x86) {
        return -1;
    }

    // Validate checksum
    uint8_t calc_cs = mhz19b_calc_checksum(resp, 8);
    if (calc_cs != resp[8]) {
        return -1;
    }

    // Extract CO2 concentration (big-endian)
    *raw = (int32_t)((uint16_t)resp[2] << 8) | resp[3];
    return 0;
}