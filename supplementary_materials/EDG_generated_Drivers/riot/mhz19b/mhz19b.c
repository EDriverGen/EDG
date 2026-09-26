#include "mhz19b.h"
#include <string.h>
#include <xtimer.h>

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

void mhz19b_init(mhz19b_t *dev, uart_t uart) {
    dev->uart = uart;
}

int32_t mhz19b_read_co2(mhz19b_t *dev, int32_t *raw) {
    uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[RESPONSE_LENGTH];

    uart_write(dev->uart, cmd, sizeof(cmd));

    xtimer_msleep(100);

    int res = uart_read(dev->uart, resp, RESPONSE_LENGTH);
    if (res != RESPONSE_LENGTH) {
        return -1;
    }

    if (resp[0] != START_BYTE) {
        return -2;
    }
    if (resp[CMD_BYTE_INDEX] != CMD_READ_CO2) {
        return -3;
    }

    uint8_t calc_checksum = calculate_checksum(resp, 8);
    if (calc_checksum != resp[CHECKSUM_INDEX]) {
        return -4;
    }

    *raw = (int32_t)((uint16_t)resp[HIGH_BYTE_INDEX] * 256 + (uint16_t)resp[LOW_BYTE_INDEX]);
    return 0;
}