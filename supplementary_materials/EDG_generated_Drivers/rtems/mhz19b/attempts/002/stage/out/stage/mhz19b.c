#include "mhz19b.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9

static uint8_t mhz19b_checksum(const uint8_t *data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_device *dev, const char *bus_handle) {
    int fd = open(bus_handle, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    dev->fd = fd;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw) {
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[MHZ19B_RESPONSE_LENGTH];
    ssize_t ret;

    ret = write(dev->fd, cmd, sizeof(cmd));
    if (ret != sizeof(cmd)) {
        return -1;
    }

    ret = read(dev->fd, resp, MHZ19B_RESPONSE_LENGTH);
    if (ret != MHZ19B_RESPONSE_LENGTH) {
        return -1;
    }

    if (resp[0] != 0xFF || resp[1] != 0x86) {
        return -1;
    }

    uint8_t calc_checksum = mhz19b_checksum(resp + 1, 7);
    if (resp[8] != calc_checksum) {
        return -1;
    }

    *raw = (int32_t)((uint16_t)resp[2] << 8 | resp[3]);
    return 0;
}