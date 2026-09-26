#include "mhz19b.h"
#include "transform.h"
#include "bus.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define CMD_READ_CO2 0x86
#define RESPONSE_LENGTH 9
#define START_BYTE 0xFF
#define CMD_BYTE_INDEX 1
#define HIGH_BYTE_INDEX 2
#define LOW_BYTE_INDEX 3
#define CHECKSUM_INDEX 8

static int uart_write(void *bus, const uint8_t *data, int len) {
    int fd = (int)(intptr_t)bus;
    return write(fd, data, len);
}

static int uart_read(void *bus, uint8_t *buf, int len) {
    int fd = (int)(intptr_t)bus;
    return read(fd, buf, len);
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -EINVAL;
    dev->bus_handle = bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw) {
    if (!dev || !raw) return -EINVAL;
    void *bus = dev->bus_handle;
    if (!bus) return -EINVAL;

    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    int ret = uart_write(bus, cmd, 9);
    if (ret != 9) return -EIO;

    uint8_t resp[9];
    memset(resp, 0, 9);
    ret = uart_read(bus, resp, 9);
    if (ret != 9) return -EIO;

    if (resp[0] != START_BYTE) return -EIO;
    if (resp[1] != CMD_READ_CO2) return -EIO;

    uint8_t checksum = 0;
    for (int i = 1; i < 8; i++) {
        checksum += resp[i];
    }
    checksum = (~checksum) + 1;
    if (checksum != resp[CHECKSUM_INDEX]) return -EIO;

    *raw = (int32_t)((uint16_t)resp[HIGH_BYTE_INDEX] * 256 + (uint16_t)resp[LOW_BYTE_INDEX]);
    return 0;
}