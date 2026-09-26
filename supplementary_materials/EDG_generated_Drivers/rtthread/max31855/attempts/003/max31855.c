#include "max31855.h"
#include <rtdevice.h>
#include <rtthread.h>
#include <stdint.h>

#define MAX31855_FAULT_BIT (1 << 16)
#define MAX31855_SCV_BIT  (1 << 2)
#define MAX31855_SCG_BIT  (1 << 1)
#define MAX31855_OC_BIT   (1 << 0)

static int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    rt_size_t ret;

    ret = rt_spi_send_then_recv(dev->spi_dev, tx_buf, 0, rx_buf, 4);
    if (ret != RT_EOK) {
        return -1;
    }

    *raw = ((uint32_t)rx_buf[0] << 24) |
           ((uint32_t)rx_buf[1] << 16) |
           ((uint32_t)rx_buf[2] << 8)  |
           ((uint32_t)rx_buf[3]);

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
    int ret;
    int16_t raw_14;
    int32_t temp;

    ret = max31855_read_raw(dev, &raw);
    if (ret != 0) {
        return -1;
    }

    if (raw & MAX31855_FAULT_BIT) {
        return 1;
    }

    raw_14 = (int16_t)((raw >> 18) & 0x3FFF);
    if (raw_14 & 0x2000) {
        raw_14 |= 0xC000;
    }

    temp = ((int32_t)raw_14) * 250 / 1000;
    *tc = temp;
    return 0;
}

int max31855_read_internal(struct max31855_device *dev, int32_t *internal)
{
    uint32_t raw;
    int ret;
    int16_t raw_12;
    int32_t temp;

    ret = max31855_read_raw(dev, &raw);
    if (ret != 0) {
        return -1;
    }

    if (raw & MAX31855_FAULT_BIT) {
        return 1;
    }

    raw_12 = (int16_t)((raw >> 4) & 0x0FFF);
    if (raw_12 & 0x0800) {
        raw_12 |= 0xF000;
    }

    temp = ((int32_t)raw_12) * 625 / 10000;
    *internal = temp;
    return 0;
}
