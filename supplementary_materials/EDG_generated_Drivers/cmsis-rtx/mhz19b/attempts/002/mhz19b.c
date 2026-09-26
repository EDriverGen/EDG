#include "mhz19b.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_SENSOR_NUM 0x01

static uint8_t mhz19b_calc_checksum(const uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_dev *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw)
{
    if (!dev || !dev->bus_handle || !raw) return -1;

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)dev->bus_handle;

    uint8_t cmd[9];
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;
    cmd[8] = mhz19b_calc_checksum(cmd, 8);

    HAL_StatusTypeDef status;
    status = HAL_UART_Transmit(huart, cmd, 9, 1000);
    if (status != HAL_OK) return -1;

    uint8_t resp[MHZ19B_RESPONSE_LENGTH];
    memset(resp, 0, sizeof(resp));
    status = HAL_UART_Receive(huart, resp, MHZ19B_RESPONSE_LENGTH, 1000);
    if (status != HAL_OK) return -1;

    if (resp[0] != MHZ19B_START_BYTE) return -1;
    if (resp[1] != MHZ19B_CMD_READ_CO2) return -1;

    uint8_t calc_checksum = mhz19b_calc_checksum(resp, 8);
    if (resp[8] != calc_checksum) return -1;

    *raw = ((int32_t)resp[2] << 8) | resp[3];
    return 0;
}