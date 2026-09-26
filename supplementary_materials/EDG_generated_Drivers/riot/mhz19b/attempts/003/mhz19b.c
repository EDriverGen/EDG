#include "mhz19b.h"
#include <string.h>
#include <xtimer.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LEN 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_COMMAND_BYTE 0x01

static uint8_t mhz19b_calc_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

void mhz19b_init(mhz19b_t *dev, uart_t uart) {
    dev->uart_dev = uart;
    uart_init(uart, 9600, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
}

int32_t mhz19b_read_co2(mhz19b_t *dev, int32_t *raw) {
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[MHZ19B_RESPONSE_LEN];
    
    /* Send command */
    uart_write(dev->uart_dev, cmd, sizeof(cmd));
    
    /* Wait for response (at least 1ms, but we use a small delay) */
    xtimer_msleep(10);
    
    /* Read response */
    size_t len = uart_read(dev->uart_dev, resp, MHZ19B_RESPONSE_LEN);
    if (len != MHZ19B_RESPONSE_LEN) {
        return -1;
    }
    
    /* Validate start byte */
    if (resp[0] != 0xFF) {
        return -1;
    }
    
    /* Validate command byte */
    if (resp[1] != 0x86) {
        return -1;
    }
    
    /* Validate checksum */
    uint8_t calc_cs = mhz19b_calc_checksum(resp, 8);
    if (calc_cs != resp[8]) {
        return -1;
    }
    
    /* Extract CO2 concentration */
    uint16_t co2 = ((uint16_t)resp[2] << 8) | resp[3];
    *raw = (int32_t)co2;
    
    return 0;
}