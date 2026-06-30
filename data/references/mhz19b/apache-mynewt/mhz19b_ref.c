#include "mhz19b_ref.h"

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) {
        sum = (uint8_t)(sum + data[i]);
    }
    return (uint8_t)((uint8_t)(~sum) + 1U);
}

static int mhz19b_write_frame(int uart_num, const uint8_t frame[MHZ19B_FRAME_LEN])
{
    for (int i = 0; i < (int)MHZ19B_FRAME_LEN; i++) {
        if (hal_uart_blocking_tx(uart_num, frame[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static int mhz19b_read_frame(int uart_num, uint8_t frame[MHZ19B_FRAME_LEN])
{
    for (int i = 0; i < (int)MHZ19B_FRAME_LEN; i++) {
        if (hal_uart_blocking_rx(uart_num, &frame[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

int mhz19b_init(struct mhz19b_device *dev, int uart_num)
{
    struct hal_uart_settings settings;
    if (dev == 0) {
        return -1;
    }
    settings.baudrate = MHZ19B_BAUD_RATE;
    settings.data_bits = 8;
    settings.stop_bits = 1;
    settings.parity = 0;
    settings.flow_ctl = 0;
    if (hal_uart_init(uart_num, 0) != 0 ||
        hal_uart_config(uart_num, &settings) != 0) {
        return -1;
    }
    dev->uart_num = uart_num;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[MHZ19B_FRAME_LEN] = {0};
    uint8_t resp[MHZ19B_FRAME_LEN] = {0};
    if (dev == 0 || ppm == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[8] = mhz19b_checksum(cmd);
    if (mhz19b_write_frame(dev->uart_num, cmd) != 0 ||
        mhz19b_read_frame(dev->uart_num, resp) != 0) {
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
    if (dev == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0U : 0x00U;
    cmd[8] = mhz19b_checksum(cmd);
    return mhz19b_write_frame(dev->uart_num, cmd);
}

int mhz19b_calibrate_zero(struct mhz19b_device *dev)
{
    uint8_t cmd[MHZ19B_FRAME_LEN] = {0};
    if (dev == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_CALIBRATE;
    cmd[8] = mhz19b_checksum(cmd);
    return mhz19b_write_frame(dev->uart_num, cmd);
}
