/*
 * MH-Z19B CO2 sensor driver for RIOT (UART)
 */
#include "mhz19b_ref.h"
#include <string.h>

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) sum += data[i];
    return (~sum) + 1;
}

static void mhz19b_rx_cb(void *arg, uint8_t data)
{
    struct mhz19b_device *dev = (struct mhz19b_device *)arg;
    if (dev->rx_pos < sizeof(dev->rx_buf))
        dev->rx_buf[dev->rx_pos++] = data;
}

int mhz19b_init(struct mhz19b_device *dev, uart_t uart)
{
    if (!dev) return -1;
    dev->uart = uart; dev->rx_pos = 0;
    if (uart_init(uart, MHZ19B_BAUD_RATE, mhz19b_rx_cb, dev) != UART_OK)
        return -1;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[9] = {0};
    if (!dev || !ppm) return -1;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0; cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    dev->rx_pos = 0;
    uart_write(dev->uart, cmd, 9);
    xtimer_msleep(200);
    if (dev->rx_pos < 9) return -1;
    if (dev->rx_buf[0] != MHZ19B_START_BYTE || dev->rx_buf[1] != MHZ19B_CMD_READ_CO2)
        return -1;
    if (dev->rx_buf[8] != mhz19b_checksum(dev->rx_buf))
        return -1;
    *ppm = (uint16_t)((uint16_t)dev->rx_buf[2] << 8 | dev->rx_buf[3]);
    return 0;
}

int mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable)
{
    uint8_t cmd[9] = {0};
    if (!dev) return -1;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0 : 0x00;
    cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    uart_write(dev->uart, cmd, 9);
    return 0;
}
