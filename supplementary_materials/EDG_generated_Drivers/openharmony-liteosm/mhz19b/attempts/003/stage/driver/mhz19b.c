#include "mhz19b.h"
#include "uart_if.h"
#include <stdint.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_COMMAND_BYTE 0x01

static uint8_t mhz19b_calculate_checksum(const uint8_t *data, uint32_t len)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)((~sum) + 1);
}

int32_t mhz19b_init(struct mhz19b_device *dev, void *bus_handle)
{
    if (!dev || !bus_handle) {
        return -1;
    }
    dev->bus_handle = bus_handle;
    return 0;
}

int32_t mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw)
{
    if (!dev || !raw) {
        return -1;
    }

    uint8_t cmd[9] = {
        0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79
    };

    int32_t ret = UartWrite((DevHandle)dev->bus_handle, cmd, 9);
    if (ret != 9) {
        return -1;
    }

    uint8_t resp[9];
    ret = UartRead((DevHandle)dev->bus_handle, resp, 9);
    if (ret != 9) {
        return -1;
    }

    if (resp[0] != 0xFF) {
        return -1;
    }
    if (resp[1] != 0x86) {
        return -1;
    }

    uint8_t calc_checksum = mhz19b_calculate_checksum(resp, 8);
    if (calc_checksum != resp[8]) {
        return -1;
    }

    *raw = (int32_t)((uint16_t)resp[2] << 8) | resp[3];
    return 0;
}