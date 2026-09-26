#include "mhz19b.h"
#include <string.h>

#include "threadx.h"
#define MHZ19B_READ_CMD {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}
#define MHZ19B_RESPONSE_SIZE 9
#define MHZ19B_TIMEOUT 1000

static uint8_t mhz19b_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_handle) {
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->huart = (UART_HandleTypeDef *)bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw) {
    if (dev == NULL || dev->huart == NULL || raw == NULL) return -1;

    uint8_t cmd[9] = MHZ19B_READ_CMD;
    uint8_t resp[MHZ19B_RESPONSE_SIZE];

    // Send command
    if (HAL_UART_Transmit(dev->huart, cmd, 9, MHZ19B_TIMEOUT) != HAL_OK) {
        return -1;
    }

    // Read response
    if (HAL_UART_Receive(dev->huart, resp, MHZ19B_RESPONSE_SIZE, MHZ19B_TIMEOUT) != HAL_OK) {
        return -1;
    }

    // Validate response
    if (resp[0] != 0xFF) return -1;
    if (resp[1] != 0x86) return -1;

    uint8_t calc_checksum = mhz19b_checksum(resp, 8);
    if (resp[8] != calc_checksum) return -1;

    // Compute CO2 concentration
    *raw = (int32_t)((uint16_t)resp[2] * 256 + (uint16_t)resp[3]);
    return 0;
}
