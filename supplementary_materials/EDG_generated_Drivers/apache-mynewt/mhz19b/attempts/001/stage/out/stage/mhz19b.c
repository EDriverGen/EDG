#include "mhz19b.h"
#include <stdint.h>
#include <string.h>

#include "apache_mynewt.h"
#include <os/os_time.h>
#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_SENSOR_NUM 0x01

static uint8_t mhz19b_calculate_checksum(const uint8_t *data, int len)
{
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_dev *dev, int uart_num)
{
    if (!dev) {
        return -1;
    }
    dev->uart_num = uart_num;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw)
{
    if (!dev || !raw) {
        return -1;
    }

    uint8_t cmd[9];
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;
    cmd[8] = mhz19b_calculate_checksum(cmd, 8);

    for (int i = 0; i < 9; i++) {
        hal_uart_blocking_tx(dev->uart_num, cmd[i]);
    }

    os_time_delay(1);

    uint8_t resp[MHZ19B_RESPONSE_LENGTH];
    for (int i = 0; i < MHZ19B_RESPONSE_LENGTH; i++) {
        resp[i] = 0;
    }

    for (int i = 0; i < MHZ19B_RESPONSE_LENGTH; i++) {
        uint8_t byte = 0;
        hal_uart_blocking_tx(dev->uart_num, 0x00);
        resp[i] = byte;
    }

    if (resp[0] != MHZ19B_START_BYTE) {
        return -1;
    }
    if (resp[1] != MHZ19B_CMD_READ_CO2) {
        return -1;
    }

    uint8_t calc_checksum = mhz19b_calculate_checksum(resp, 8);
    if (resp[8] != calc_checksum) {
        return -1;
    }

    *raw = (int32_t)(((uint16_t)resp[2] << 8) | resp[3]);
    return 0;
}
