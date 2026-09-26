#include "mhz19b.h"
#include <string.h>
#include <xtimer.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9
#define MHZ19B_BAUDRATE 9600

static uint8_t mhz19b_calc_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

void mhz19b_init(mhz19b_t *dev, uart_t uart) {
    dev->uart = uart;
    uart_init(uart, MHZ19B_BAUDRATE);
}

int32_t mhz19b_read_co2(mhz19b_t *dev, int32_t *raw) {
    uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[MHZ19B_RESPONSE_LENGTH];
    
    uart_write(dev->uart, cmd, sizeof(cmd));
    xtimer_msleep(100);
    
    size_t read_len = uart_read(dev->uart, resp, MHZ19B_RESPONSE_LENGTH);
    if (read_len != MHZ19B_RESPONSE_LENGTH) {
        return -1;
    }
    
    if (resp[0] != 0xFF || resp[1] != 0x86) {
        return -1;
    }
    
    uint8_t calc_cs = mhz19b_calc_checksum(resp, 8);
    if (resp[8] != calc_cs) {
        return -1;
    }
    
    *raw = (int32_t)((uint16_t)resp[2] * 256 + (uint16_t)resp[3]);
    return 0;
}