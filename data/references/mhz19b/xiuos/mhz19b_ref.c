/*
 * MH-Z19B CO2 sensor driver for XiUOS
 */
#include "mhz19b_ref.h"
#include <string.h>

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) sum += data[i];
    return (~sum) + 1;
}

int mhz19b_init(struct mhz19b_device *dev, const char *uart_path)
{
    if (!dev || !uart_path) return -1;
    dev->uart_fd = PrivOpen(uart_path, O_RDWR);
    if (dev->uart_fd < 0) return -1;
    return 0;
}

void mhz19b_deinit(struct mhz19b_device *dev)
{ if (dev && dev->uart_fd >= 0) { PrivClose(dev->uart_fd); dev->uart_fd = -1; } }

int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[9] = {0}, resp[9] = {0};
    if (!dev || dev->uart_fd < 0 || !ppm) return -1;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0; cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    if (PrivWrite(dev->uart_fd, cmd, 9) != 9) return -1;
    PrivTaskDelay(100);
    if (PrivRead(dev->uart_fd, resp, 9) != 9) return -1;
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
    if (!dev || dev->uart_fd < 0) return -1;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0 : 0x00;
    cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    return (PrivWrite(dev->uart_fd, cmd, 9) == 9) ? 0 : -1;
}
