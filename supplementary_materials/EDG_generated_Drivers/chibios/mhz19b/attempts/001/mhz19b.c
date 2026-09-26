#include "mhz19b.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#define MHZ19B_READ_CMD_LEN 9
#define MHZ19B_RESP_LEN 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_CMD_READ_CO2 0x86

static uint8_t mhz19b_calc_checksum(const uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (~sum) + 1;
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw)
{
    uint8_t cmd[MHZ19B_READ_CMD_LEN] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[MHZ19B_RESP_LEN];
    msg_t status;
    (void)dev;

    // Send command
    status = uartSendTimeout((UARTDriver *)dev->bus_handle, cmd, MHZ19B_READ_CMD_LEN, TIME_INFINITE);
    if (status != MSG_OK) {
        return -1;
    }

    // Read response
    status = uartReceiveTimeout((UARTDriver *)dev->bus_handle, resp, MHZ19B_RESP_LEN, TIME_INFINITE);
    if (status != MSG_OK) {
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
    uint8_t calc_cs = mhz19b_calc_checksum(resp, 8);
    if (calc_cs != resp[8]) {
        return -1;
    }

    // Extract CO2 value
    *raw = (int32_t)((uint16_t)resp[2] << 8) | resp[3];
    return 0;
}