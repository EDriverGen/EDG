/*
 * MH-Z19B CO2 sensor driver for RT-Thread (UART)
 */
#include "mhz19b_ref.h"
#include <string.h>

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) sum += data[i];
    return (~sum) + 1;
}

rt_err_t mhz19b_init(struct mhz19b_device *dev, const char *uart_name)
{
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
    if (dev == RT_NULL || uart_name == RT_NULL) return -RT_EINVAL;
    dev->uart_name = uart_name;
    dev->serial = rt_device_find(uart_name);
    if (dev->serial == RT_NULL) return -RT_ENOSYS;
    cfg.baud_rate = MHZ19B_BAUD_RATE;
    rt_device_control(dev->serial, RT_DEVICE_CTRL_CONFIG, &cfg);
    rt_device_open(dev->serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    return RT_EOK;
}

rt_err_t mhz19b_read_co2(struct mhz19b_device *dev, rt_uint16_t *ppm)
{
    uint8_t cmd[9] = {0}, resp[9] = {0};
    if (dev == RT_NULL || dev->serial == RT_NULL || ppm == RT_NULL) return -RT_EINVAL;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0; cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    rt_device_write(dev->serial, 0, cmd, 9);
    rt_thread_mdelay(100);
    rt_size_t n = rt_device_read(dev->serial, 0, resp, 9);
    if (n != 9) return -RT_EIO;
    if (resp[0] != MHZ19B_START_BYTE || resp[1] != MHZ19B_CMD_READ_CO2)
        return -RT_EIO;
    if (resp[8] != mhz19b_checksum(resp))
        return -RT_EIO;
    *ppm = (uint16_t)((uint16_t)resp[2] << 8 | resp[3]);
    return RT_EOK;
}

rt_err_t mhz19b_set_abc(struct mhz19b_device *dev, rt_uint8_t enable)
{
    uint8_t cmd[9] = {0};
    if (dev == RT_NULL || dev->serial == RT_NULL) return -RT_EINVAL;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0 : 0x00;
    cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    rt_device_write(dev->serial, 0, cmd, 9);
    return RT_EOK;
}

rt_err_t mhz19b_calibrate_zero(struct mhz19b_device *dev)
{
    uint8_t cmd[9] = {0};
    if (dev == RT_NULL || dev->serial == RT_NULL) return -RT_EINVAL;
    cmd[0] = MHZ19B_START_BYTE; cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_CALIBRATE;
    cmd[8] = mhz19b_checksum(cmd);
    rt_device_write(dev->serial, 0, cmd, 9);
    return RT_EOK;
}
