#include "mhz19b.h"
#include <rtdevice.h>
#include <stdint.h>
#include <string.h>

static rt_device_t uart_dev = RT_NULL;

int mhz19b_init(struct mhz19b_device *dev, void *uart1)
{
    (void)dev;
    uart_dev = (rt_device_t)uart1;
    return 0;
}

static uint8_t mhz19b_checksum(uint8_t *buf, int len)
{
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += buf[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw)
{
    (void)dev;
    uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[9];
    int ret;

    if (uart_dev == RT_NULL) return -1;

    ret = rt_device_write(uart_dev, 0, cmd, sizeof(cmd));
    if (ret != sizeof(cmd)) return -1;

    rt_thread_mdelay(100);

    ret = rt_device_read(uart_dev, 0, resp, sizeof(resp));
    if (ret != sizeof(resp)) return -1;

    if (resp[0] != 0xFF) return -1;
    if (resp[1] != 0x86) return -1;

    uint8_t calc_checksum = mhz19b_checksum(&resp[1], 7);
    if (resp[8] != calc_checksum) return -1;

    *raw = (int32_t)((uint16_t)resp[2] * 256 + (uint16_t)resp[3]);
    return 0;
}