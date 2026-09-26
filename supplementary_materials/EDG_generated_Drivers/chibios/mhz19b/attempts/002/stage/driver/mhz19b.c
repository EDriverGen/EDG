#include "mhz19b.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_CMD_BYTE_INDEX 1
#define MHZ19B_CO2_HIGH_INDEX 2
#define MHZ19B_CO2_LOW_INDEX 3
#define MHZ19B_CHECKSUM_INDEX 8

static uint8_t mhz19b_calculate_checksum(const uint8_t *data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (~sum) + 1;
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_handle) {
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw) {
    if (dev == NULL || raw == NULL) return -1;
    SerialDriver *sdp = (SerialDriver *)dev->bus_handle;
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[9];
    msg_t status;
    
    status = sdWriteTimeout(sdp, cmd, sizeof(cmd), TIME_INFINITE);
    if (status != MSG_OK) return -1;
    
    status = sdReadTimeout(sdp, resp, sizeof(resp), TIME_INFINITE);
    if (status != MSG_OK) return -1;
    
    if (resp[0] != MHZ19B_START_BYTE) return -1;
    if (resp[1] != MHZ19B_CMD_READ_CO2) return -1;
    
    uint8_t calc_checksum = mhz19b_calculate_checksum(resp, 8);
    if (calc_checksum != resp[MHZ19B_CHECKSUM_INDEX]) return -1;
    
    *raw = (int32_t)((uint16_t)resp[MHZ19B_CO2_HIGH_INDEX] * 256 + (uint16_t)resp[MHZ19B_CO2_LOW_INDEX]);
    return 0;
}