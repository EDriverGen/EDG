#include "mhz19b.h"
#include <string.h>

#include "threadx.h"
#define CMD_READ_CO2 0x86
#define CMD_CAL_ZERO 0x87
#define CMD_CAL_SPAN 0x88
#define CMD_SELF_CAL 0x79
#define CMD_RANGE    0x99
#define RESP_START_BYTE 0xFF
#define RESP_CMD_BYTE   0x86
#define RESP_LENGTH 9
#define TIMEOUT_MS 1000

static uint8_t calc_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->huart = (UART_HandleTypeDef *)bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw) {
    if (!dev || !dev->huart || !raw) return -1;

    uint8_t cmd[9] = {0xFF, 0x01, CMD_READ_CO2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cmd[8] = calc_checksum(cmd, 8);

    if (HAL_UART_Transmit(dev->huart, cmd, 9, TIMEOUT_MS) != HAL_OK) {
        return -1;
    }

    uint8_t resp[RESP_LENGTH];
    memset(resp, 0, RESP_LENGTH);
    if (HAL_UART_Receive(dev->huart, resp, RESP_LENGTH, TIMEOUT_MS) != HAL_OK) {
        return -1;
    }

    if (resp[0] != RESP_START_BYTE) return -1;
    if (resp[1] != RESP_CMD_BYTE) return -1;

    uint8_t calc = calc_checksum(resp, 8);
    if (resp[8] != calc) return -1;

    *raw = (int32_t)((uint16_t)resp[2] << 8 | resp[3]);
    return 0;
}
