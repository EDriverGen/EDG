/*
 * MH-Z19B CO2 sensor driver for OpenHarmony HDF
 */
#include "mhz19b_ref.h"
#include "hdf_log.h"
#include "osal_time.h"
#define HDF_LOG_TAG mhz19b

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) sum += data[i];
    return (~sum) + 1;
}

int32_t mhz19b_init(struct mhz19b_device *dev, uint32_t port)
{
    if (!dev) return HDF_ERR_INVALID_PARAM;
    dev->uart_handle = UartOpen(port);
    if (!dev->uart_handle) return HDF_FAILURE;
    struct UartAttribute attr = { .dataBits=UART_ATTR_DATABIT_8, .parity=UART_ATTR_PARITY_NONE,
                                  .stopBits=UART_ATTR_STOPBIT_1 };
    UartSetAttribute(dev->uart_handle, &attr);
    UartSetBaud(dev->uart_handle, MHZ19B_BAUD_RATE);
    return HDF_SUCCESS;
}

void mhz19b_deinit(struct mhz19b_device *dev)
{ if (dev && dev->uart_handle) { UartClose(dev->uart_handle); dev->uart_handle = NULL; } }

int32_t mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[9] = {0}, resp[9] = {0};
    if (!dev || !dev->uart_handle || !ppm) return HDF_ERR_INVALID_PARAM;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0; cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    if (UartWrite(dev->uart_handle, cmd, 9) != HDF_SUCCESS) return HDF_FAILURE;
    OsalMSleep(100);
    if (UartRead(dev->uart_handle, resp, 9) != 9) return HDF_FAILURE;
    if (resp[0] != MHZ19B_START_BYTE || resp[1] != MHZ19B_CMD_READ_CO2)
        return HDF_FAILURE;
    if (resp[8] != mhz19b_checksum(resp))
        return HDF_FAILURE;
    *ppm = (uint16_t)((uint16_t)resp[2] << 8 | resp[3]);
    return HDF_SUCCESS;
}

int32_t mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable)
{
    uint8_t cmd[9] = {0};
    if (!dev || !dev->uart_handle) return HDF_ERR_INVALID_PARAM;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0 : 0x00;
    cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    return UartWrite(dev->uart_handle, cmd, 9);
}
