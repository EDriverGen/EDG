#include "max31855.h"
#include <rtthread.h>
#include <rtdevice.h>

int max31855_init(struct max31855_device *dev, struct rt_spi_device *spi)
{
    if (dev == RT_NULL || spi == RT_NULL) {
        return -1;
    }
    dev->spi = spi;
    /* No bus traffic needed; device continuously converts.
     * Wait 200 ms for power-up before first read. */
    rt_thread_mdelay(200);
    return 0;
}

static int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    rt_err_t ret;

    if (dev == RT_NULL || dev->spi == RT_NULL) {
        return -1;
    }

    /* Stream read: send 0 bytes, receive 4 bytes */
    ret = rt_spi_send_then_recv(dev->spi, RT_NULL, 0, rx_buf, 4);
    if (ret != RT_EOK) {
        return -1;
    }

    *raw = ((uint32_t)rx_buf[0] << 24) |
           ((uint32_t)rx_buf[1] << 16) |
           ((uint32_t)rx_buf[2] << 8) |
           ((uint32_t)rx_buf[3] << 0);
    return 0;
}

int max31855_read_thermocouple(struct max31855_device *dev, int32_t *tc)
{
    uint32_t raw;
    int ret;
    int16_t raw_14;
    int32_t temp;

    if (dev == RT_NULL || tc == RT_NULL) {
        return -1;
    }

    ret = max31855_read_raw(dev, &raw);
    if (ret != 0) {
        return ret;
    }

    /* Check fault bit D16 */
    if (raw & (1 << 16)) {
        return 1; /* fault */
    }

    /* Extract thermocouple temperature: bits D[31:18] -> raw_14 */
    raw_14 = (int16_t)((raw >> 18) & 0x3FFF);
    /* Sign-extend from 14 bits */
    if (raw_14 & 0x2000) {
        raw_14 |= 0xC000;
    }

    /* Conversion: raw_14 * 0.25 -> milli_degC */
    /* integer expression: (((raw_14 >> 13) & 1) * (-8192) + ((raw_14 & 0x1FFF) ^ ((raw_14 >> 13) & 1) * 0x2000)) * 250 // 1000 */
    {
        int32_t sign = (raw_14 >> 13) & 1;
        int32_t magnitude = raw_14 & 0x1FFF;
        if (sign) {
            magnitude ^= 0x2000;
        }
        temp = (sign * (-8192) + magnitude) * 250 / 1000;
    }
    *tc = temp;
    return 0;
}

int max31855_read_internal(struct max31855_device *dev, int32_t *internal)
{
    uint32_t raw;
    int ret;
    int16_t raw_12;
    int32_t temp;

    if (dev == RT_NULL || internal == RT_NULL) {
        return -1;
    }

    ret = max31855_read_raw(dev, &raw);
    if (ret != 0) {
        return ret;
    }

    /* Check fault bit D16 */
    if (raw & (1 << 16)) {
        return 1; /* fault */
    }

    /* Extract internal temperature: bits D[15:4] -> raw_12 */
    raw_12 = (int16_t)((raw >> 4) & 0x0FFF);
    /* Sign-extend from 12 bits */
    if (raw_12 & 0x0800) {
        raw_12 |= 0xF000;
    }

    /* Conversion: raw_12 * 0.0625 -> milli_degC */
    /* integer expression: (((raw_12 >> 11) & 1) * (-2048) + ((raw_12 & 0x7FF) ^ ((raw_12 >> 11) & 1) * 0x800)) * 625 // 10000 */
    {
        int32_t sign = (raw_12 >> 11) & 1;
        int32_t magnitude = raw_12 & 0x7FF;
        if (sign) {
            magnitude ^= 0x800;
        }
        temp = (sign * (-2048) + magnitude) * 625 / 10000;
    }
    *internal = temp;
    return 0;
}
