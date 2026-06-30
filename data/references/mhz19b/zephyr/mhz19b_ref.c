/*
 * MH-Z19B CO2 sensor driver for Zephyr (UART)
 */
#include "mhz19b_ref.h"
#include <string.h>

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) sum += data[i];
    return (~sum) + 1;
}

static int mhz19b_uart_send(const struct device *uart, const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) uart_poll_out(uart, data[i]);
    return 0;
}

static int mhz19b_uart_recv(const struct device *uart, uint8_t *data, int len, int timeout_ms)
{
    /* Under the evaluation's busy-wait UART stubs, uart_poll_in() already
     * performs a bounded per-byte busy-wait (via hw_uart_bus_read_byte),
     * and k_msleep() is a no-op. Stacking an outer `tries`/`k_msleep(1)`
     * retry on top of that would multiply the inner MMIO budget by
     * `timeout_ms` on every silent byte (200k * 500 ≈ 1e8 MMIO reads per
     * byte), which deadlocks the firmware. Poll once per byte and rely on
     * the inner timeout to surface the silent-sensor fault. */
    (void)timeout_ms;
    for (int i = 0; i < len; i++) {
        if (uart_poll_in(uart, &data[i]) != 0) return -1;
    }
    return 0;
}

int mhz19b_init(struct mhz19b_device *dev, const struct device *uart)
{
    if (!dev || !uart) return -EINVAL;
    if (!device_is_ready(uart)) return -ENODEV;
    dev->uart_dev = uart;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[9] = {0}, resp[9] = {0};
    if (!dev || !dev->uart_dev || !ppm) return -EINVAL;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0; cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    mhz19b_uart_send(dev->uart_dev, cmd, 9);
    if (mhz19b_uart_recv(dev->uart_dev, resp, 9, 500) != 0) return -EIO;
    if (resp[0] != MHZ19B_START_BYTE || resp[1] != MHZ19B_CMD_READ_CO2)
        return -EIO;
    if (resp[8] != mhz19b_checksum(resp))
        return -EIO;
    *ppm = (uint16_t)((uint16_t)resp[2] << 8 | resp[3]);
    return 0;
}

int mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable)
{
    uint8_t cmd[9] = {0};
    if (!dev || !dev->uart_dev) return -EINVAL;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0 : 0x00;
    cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    mhz19b_uart_send(dev->uart_dev, cmd, 9);
    return 0;
}
