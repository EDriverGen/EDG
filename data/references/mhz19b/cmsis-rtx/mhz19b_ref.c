#include "mhz19b_ref.h"

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) {
        sum = (uint8_t)(sum + data[i]);
    }
    return (uint8_t)((uint8_t)(~sum) + 1U);
}

int mhz19b_init(struct mhz19b_device *dev, UART_HandleTypeDef *uart)
{
    if (dev == 0 || uart == 0) {
        return -1;
    }
    if (HAL_UART_Init(uart) != HAL_OK) {
        return -1;
    }
    dev->uart = uart;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[MHZ19B_FRAME_LEN] = {0};
    uint8_t resp[MHZ19B_FRAME_LEN] = {0};
    if (dev == 0 || dev->uart == 0 || ppm == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[8] = mhz19b_checksum(cmd);
    if (HAL_UART_Transmit(dev->uart, cmd, MHZ19B_FRAME_LEN, 100) != HAL_OK) {
        return -1;
    }
    if (HAL_UART_Receive(dev->uart, resp, MHZ19B_FRAME_LEN, 200) != HAL_OK) {
        return -1;
    }
    if (resp[0] != MHZ19B_START_BYTE || resp[1] != MHZ19B_CMD_READ_CO2) {
        return -1;
    }
    if (resp[8] != mhz19b_checksum(resp)) {
        return -1;
    }
    *ppm = (uint16_t)(((uint16_t)resp[2] << 8) | resp[3]);
    return 0;
}

int mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable)
{
    uint8_t cmd[MHZ19B_FRAME_LEN] = {0};
    if (dev == 0 || dev->uart == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0U : 0x00U;
    cmd[8] = mhz19b_checksum(cmd);
    return HAL_UART_Transmit(dev->uart, cmd, MHZ19B_FRAME_LEN, 100) == HAL_OK ? 0 : -1;
}

int mhz19b_calibrate_zero(struct mhz19b_device *dev)
{
    uint8_t cmd[MHZ19B_FRAME_LEN] = {0};
    if (dev == 0 || dev->uart == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_CALIBRATE;
    cmd[8] = mhz19b_checksum(cmd);
    return HAL_UART_Transmit(dev->uart, cmd, MHZ19B_FRAME_LEN, 100) == HAL_OK ? 0 : -1;
}
