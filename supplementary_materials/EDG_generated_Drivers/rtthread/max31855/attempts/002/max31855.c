#include "max31855.h"
#include <rtdevice.h>
#include <rtthread.h>
#include <stdint.h>

static int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    rt_err_t ret = rt_spi_send_then_recv(dev->spi_dev, tx_buf, 0, rx_buf, 4);
    if (ret != RT_EOK) {
        return -1;
    }
    *raw = ((uint32_t)rx_buf[0] << 24) | ((uint32_t)rx_buf[1] << 16) |
           ((uint32_t)rx_buf[2] << 8) | rx_buf[3];
    return 0;
}

int max31855_init(struct max31855_device *dev, struct rt_spi_device *spi)
{
    dev->spi_dev = spi;
    rt_thread_mdelay(200);
    return 0;
}

int max31855_read_thermocouple(struct max31855_device *dev, int32_t *tc)
{
    uint32_t raw;
    int ret = max31855_read_raw(dev, &raw);
    if (ret != 0) {
        return -1;
    }
    if (raw & 0x00010000) {
        return -1;
    }
    uint16_t raw_14 = (raw >> 18) & 0x3FFF;
    int32_t sign = (raw_14 >> 13) & 1;
    int32_t magnitude = raw_14 & 0x1FFF;
    int32_t val = sign * (-8192) + (magnitude ^ (sign * 0x2000));
    *tc = (val * 250) / 1000;
    return 0;
}

int max31855_read_internal(struct max31855_device *dev, int32_t *internal)
{
    uint32_t raw;
    int ret = max31855_read_raw(dev, &raw);
    if (ret != 0) {
        return -1;
    }
    if (raw & 0x00010000) {
        return -1;
    }
    uint16_t raw_12 = (raw >> 4) & 0xFFF;
    int32_t sign = (raw_12 >> 11) & 1;
    int32_t magnitude = raw_12 & 0x7FF;
    int32_t val = sign * (-2048) + (magnitude ^ (sign * 0x800));
    *internal = (val * 625) / 10000;
    return 0;
}
