#include "mhz19b.h"
#include <stddef.h>
#include "stm32f1xx_hal.h"

#include "tobudos.h"
#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_CMD_BYTE_INDEX 1
#define MHZ19B_HIGH_BYTE_INDEX 2
#define MHZ19B_LOW_BYTE_INDEX 3
#define MHZ19B_CHECKSUM_INDEX 8

static uint8_t mhz19b_calculate_checksum(const uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (~sum) + 1;
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->huart = (UART_HandleTypeDef *)bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw)
{
    if (dev == NULL || dev->huart == NULL || raw == NULL) {
        return -1;
    }

    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[9];

    // Send command
    if (HAL_UART_Transmit(dev->huart, cmd, 9, 100) != HAL_OK) {
        return -1;
    }

    // Read response
    if (HAL_UART_Receive(dev->huart, resp, 9, 100) != HAL_OK) {
        return -1;
    }

    // Validate start byte
    if (resp[0] != MHZ19B_START_BYTE) {
        return -1;
    }

    // Validate command byte
    if (resp[1] != MHZ19B_CMD_READ_CO2) {
        return -1;
    }

    // Validate checksum
    uint8_t calc_checksum = mhz19b_calculate_checksum(resp, 8);
    if (calc_checksum != resp[MHZ19B_CHECKSUM_INDEX]) {
        return -1;
    }

    // Extract CO2 value
    *raw = (int32_t)((uint16_t)resp[MHZ19B_HIGH_BYTE_INDEX] << 8) | resp[MHZ19B_LOW_BYTE_INDEX];

    return 0;
}
