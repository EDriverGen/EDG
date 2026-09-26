#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define MAX31855_CS_INACTIVE_TIME_MS 1

static int max31855_read_raw(const struct device *dev, uint32_t *raw)
{
    struct spi_dt_spec spec = {
        .bus = dev,
        .config = {
            .frequency = 4000000,
            .operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
            .slave = 0,
            .cs = {
                .gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi1), cs_gpios, 0),
                .delay = 0
            }
        }
    };
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    const struct spi_buf tx_bufs = { .buf = tx_buf, .len = 4 };
    const struct spi_buf rx_bufs = { .buf = rx_buf, .len = 4 };
    const struct spi_buf_set tx = { .buffers = &tx_bufs, .count = 1 };
    const struct spi_buf_set rx = { .buffers = &rx_bufs, .count = 1 };
    int ret = spi_transceive_dt(&spec, &tx, &rx);
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
    k_msleep(200);
    return 0;
}

int max31855_read_temperatures(const struct device *dev, int32_t *thermocouple_val, int32_t *temp_local_val)
{
    uint32_t raw;
    int ret = max31855_read_raw(dev, &raw);
    if (ret < 0) {
        return ret;
    }
    if (raw & (1 << 16)) {
        return -EIO;
    }
    uint16_t raw_14 = (uint16_t)((raw >> 18) & 0x3FFF);
    uint16_t raw_12 = (uint16_t)((raw >> 4) & 0xFFF);
    int32_t thermocouple_temp = (((raw_14 >> 13) & 1) ? (-8192 + (int32_t)(raw_14 & 0x1FFF)) : (int32_t)(raw_14 & 0x1FFF)) * 250;
    int32_t internal_temp = (((raw_12 >> 11) & 1) ? (-2048 + (int32_t)(raw_12 & 0x7FF)) : (int32_t)(raw_12 & 0x7FF)) * 625 / 10;
    *thermocouple_val = thermocouple_temp;
    *temp_local_val = internal_temp;
    return 0;
}
