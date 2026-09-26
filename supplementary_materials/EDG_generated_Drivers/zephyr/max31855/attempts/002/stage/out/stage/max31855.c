#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/sys/byteorder.h>
#define MAX31855_CS_INACTIVE_TIME_MS 1

static int max31855_read_raw(const struct device *dev, uint32_t *raw)
{
    const struct spi_dt_spec *spec = (const struct spi_dt_spec *)dev->data;
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    const struct spi_buf tx_bufs = { .buf = tx_buf, .len = 4 };
    const struct spi_buf rx_bufs = { .buf = rx_buf, .len = 4 };
    const struct spi_buf_set tx = { .buffers = &tx_bufs, .count = 1 };
    const struct spi_buf_set rx = { .buffers = &rx_bufs, .count = 1 };
    int ret;

    ret = spi_transceive_dt(spec, &tx, &rx);
    if (ret < 0) {
        return ret;
    }

    *raw = ((uint32_t)rx_buf[0] << 24) | ((uint32_t)rx_buf[1] << 16) |
           ((uint32_t)rx_buf[2] << 8) | rx_buf[3];

    return 0;
}

int max31855_init(const struct device *dev)
{
    (void)dev;
    return 0;
}

int max31855_read_temperatures(const struct device *dev, int32_t *thermocouple_val, int32_t *temp_local_val)
{
    uint32_t raw;
    int ret;
    int16_t raw_14, raw_12;
    int32_t thermocouple, temp_local;

    ret = max31855_read_raw(dev, &raw);
    if (ret < 0) {
        return ret;
    }

    if (raw & 0x00010000) {
        return 1;
    }

    raw_14 = (int16_t)((raw >> 18) & 0x3FFF);
    if (raw_14 & 0x2000) {
        raw_14 |= 0xC000;
    }
    thermocouple = (((raw_14 >> 13) & 1) * (-8192) + ((raw_14 & 0x1FFF) ^ ((raw_14 >> 13) & 1) * 0x2000)) * 250 / 1000;

    raw_12 = (int16_t)((raw >> 4) & 0xFFF);
    if (raw_12 & 0x800) {
        raw_12 |= 0xF000;
    }
    temp_local = (((raw_12 >> 11) & 1) * (-2048) + ((raw_12 & 0x7FF) ^ ((raw_12 >> 11) & 1) * 0x800)) * 625 / 10000;

    *thermocouple_val = thermocouple;
    *temp_local_val = temp_local;

    return 0;
}
