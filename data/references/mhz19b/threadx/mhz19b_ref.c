/*
 * MH-Z19B CO2 sensor driver for ThreadX
 */
#include "mhz19b_ref.h"
#include <stddef.h>
#include <string.h>

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) sum += data[i];
    return (~sum) + 1;
}

int mhz19b_init(struct mhz19b_device *dev, const struct mhz19b_uart_ops *ops, void *ctx)
{
    if (!dev || !ops || !ops->uart_send || !ops->uart_recv) return -1;
    dev->ops = ops; dev->ctx = ctx;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[9] = {0}, resp[9] = {0};
    if (!dev || !dev->ops || !ppm) return -1;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0; cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    if (dev->ops->uart_send(dev->ctx, cmd, 9) != 0) return -1;
    if (dev->ops->uart_recv(dev->ctx, resp, 9, 500) != 0) return -1;
    if (resp[0] != MHZ19B_START_BYTE || resp[1] != MHZ19B_CMD_READ_CO2)
        return -1;
    if (resp[8] != mhz19b_checksum(resp))
        return -1;
    *ppm = (uint16_t)((uint16_t)resp[2] << 8 | resp[3]);
    return 0;
}

int mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable)
{
    uint8_t cmd[9] = {0};
    if (!dev || !dev->ops) return -1;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0 : 0x00;
    cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    return dev->ops->uart_send(dev->ctx, cmd, 9);
}
