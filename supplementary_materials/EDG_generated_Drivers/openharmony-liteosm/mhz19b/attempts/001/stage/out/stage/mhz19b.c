#include "mhz19b.h"
#include "uart_if.h"
#include <stdint.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9

static uint8_t mhz19b_calculate_checksum(const uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int32_t mhz19b_init(struct mhz19b_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    return 0;
}

int32_t mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw)
{
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[9];
    int32_t ret;

    ret = UartWrite((DevHandle)dev->bus_handle, cmd, 9);
    if (ret != 9) {
        return -1;
    }

    ret = UartRead((DevHandle)dev->bus_handle, resp, 9);
    if (ret != 9) {
        return -1;
    }

    if (resp[0] != 0xFF || resp[1] != 0x86) {
        return -1;
    }

    uint8_t calc_checksum = mhz19b_calculate_checksum(resp, 8);
    if (calc_checksum != resp[8]) {
        return -1;
    }

    *raw = (int32_t)((uint16_t)resp[2] * 256 + (uint16_t)resp[3]);
    return 0;
}