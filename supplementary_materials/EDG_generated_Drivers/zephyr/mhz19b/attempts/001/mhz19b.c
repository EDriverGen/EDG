#include "mhz19b.h"
#include <zephyr/drivers/uart.h>
#include <errno.h>
#include <stdint.h>

#define MHZ19B_READ_CMD_LEN 9
#define MHZ19B_RESP_LEN 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_CMD_READ_CO2 0x86

static int mhz19b_send_cmd(const struct device *dev, const uint8_t *cmd, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        int ret = uart_poll_out(dev, cmd[i]);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

static int mhz19b_read_response(const struct device *dev, uint8_t *buf, size_t len)
{
    int total = 0;
    while (total < (int)len) {
        int ret = uart_fifo_read(dev, buf + total, len - total);
        if (ret < 0) {
            return ret;
        }
        total += ret;
    }
    return 0;
}

int mhz19b_init(const struct device *dev)
{
    if (!device_is_ready(dev)) {
        return -ENODEV;
    }
    return 0;
}

int mhz19b_read_co2(const struct device *dev, int32_t *raw)
{
    uint8_t cmd[MHZ19B_READ_CMD_LEN] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[MHZ19B_RESP_LEN];
    int ret;

    ret = mhz19b_send_cmd(dev, cmd, MHZ19B_READ_CMD_LEN);
    if (ret < 0) {
        return ret;
    }

    ret = mhz19b_read_response(dev, resp, MHZ19B_RESP_LEN);
    if (ret < 0) {
        return ret;
    }

    if (resp[0] != MHZ19B_START_BYTE) {
        return -EIO;
    }
    if (resp[1] != MHZ19B_CMD_READ_CO2) {
        return -EIO;
    }

    uint8_t checksum = 0;
    for (int i = 1; i < 8; i++) {
        checksum += resp[i];
    }
    checksum = (~checksum) + 1;
    if (checksum != resp[8]) {
        return -EIO;
    }

    *raw = ((int32_t)resp[2] << 8) | resp[3];
    return 0;
}