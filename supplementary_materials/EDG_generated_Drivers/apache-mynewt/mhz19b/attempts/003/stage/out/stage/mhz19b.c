#include "mhz19b.h"
#include <stdint.h>
#include <string.h>

#include "apache_mynewt.h"
#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9

static uint8_t mhz19b_calc_checksum(const uint8_t *packet, int len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += packet[i];
    }
    return (~sum) + 1;
}

int mhz19b_init(struct mhz19b_dev *dev, int uart_num) {
    dev->uart_num = uart_num;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw) {
    uint8_t cmd[9];
    cmd[0] = 0xFF;
    cmd[1] = 0x01;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;
    cmd[8] = mhz19b_calc_checksum(cmd, 8);

    for (int i = 0; i < 9; i++) {
        hal_uart_blocking_tx(dev->uart_num, cmd[i]);
    }

    uint8_t resp[9];
    for (int i = 0; i < 9; i++) {
        resp[i] = 0;
    }

    if (resp[0] != 0xFF) {
        return -1;
    }
    if (resp[1] != MHZ19B_CMD_READ_CO2) {
        return -1;
    }

    uint8_t calc_cs = mhz19b_calc_checksum(resp, 8);
    if (calc_cs != resp[8]) {
        return -1;
    }

    *raw = ((int32_t)resp[2] << 8) | resp[3];
    return 0;
}
