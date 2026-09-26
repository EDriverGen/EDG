#include "mhz19b.h"
#include <zephyr/drivers/uart.h>
#include <errno.h>

#define MHZ19B_CMD_READ_CO2 {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}
#define MHZ19B_RESPONSE_LENGTH 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_CMD_BYTE 0x86

static int mhz19b_write_command(const struct device *dev, const uint8_t *cmd, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = cmd[i];
        int ret = uart_fifo_fill(dev, &byte, 1);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

static int mhz19b_read_response(const struct device *dev, uint8_t *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        int ret = uart_fifo_read(dev, buf + total, len - total);
        if (ret < 0) {
            return ret;
        }
        total += ret;
    }
    return 0;
}

static uint8_t mhz19b_checksum(const uint8_t *data, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (~sum) + 1;
}

int mhz19b_init(const struct device *dev)
{
    if (!dev) {
        return -EINVAL;
    }
    return 0;
}

int mhz19b_read_co2(const struct device *dev, int32_t *raw)
{
    if (!dev || !raw) {
        return -EINVAL;
    }

    uint8_t cmd[] = MHZ19B_CMD_READ_CO2;
    int ret = mhz19b_write_command(dev, cmd, sizeof(cmd));
    if (ret < 0) {
        return ret;
    }

    uint8_t resp[MHZ19B_RESPONSE_LENGTH];
    ret = mhz19b_read_response(dev, resp, sizeof(resp));
    if (ret < 0) {
        return ret;
    }

    if (resp[0] != MHZ19B_START_BYTE) {
        return -EIO;
    }
    if (resp[1] != MHZ19B_CMD_BYTE) {
        return -EIO;
    }

    uint8_t calc_cs = mhz19b_checksum(resp + 1, 7);
    if (calc_cs != resp[8]) {
        return -EIO;
    }

    *raw = ((int32_t)resp[2] << 8) | resp[3];
    return 0;
}