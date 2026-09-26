#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/sys/byteorder.h>
#define ADXL345_REG_POWER_CTL 0x2D
#define ADXL345_REG_DATAX0    0x32
#define ADXL345_READ_CMD(reg) (0x80 | (reg))
#define ADXL345_MB_CMD(reg)   (0xC0 | (reg))

static int adxl345_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
    const struct spi_dt_spec *spec = (const struct spi_dt_spec *)dev;
    uint8_t tx_buf[2] = {reg, val};
    const struct spi_buf tx_bufs = {.buf = tx_buf, .len = 2};
    const struct spi_buf_set tx = {.buffers = &tx_bufs, .count = 1};
    return spi_transceive_dt(spec, &tx, NULL);
}

static int adxl345_read_burst(const struct device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    const struct spi_dt_spec *spec = (const struct spi_dt_spec *)dev;
    uint8_t cmd = ADXL345_MB_CMD(reg);
    uint8_t tx_buf[1] = {cmd};
    uint8_t rx_buf[1 + len];
    const struct spi_buf tx_bufs = {.buf = tx_buf, .len = 1};
    const struct spi_buf rx_bufs = {.buf = rx_buf, .len = 1 + len};
    const struct spi_buf_set tx = {.buffers = &tx_bufs, .count = 1};
    const struct spi_buf_set rx = {.buffers = &rx_bufs, .count = 1};
    int ret = spi_transceive_dt(spec, &tx, &rx);
    if (ret == 0) {
        for (size_t i = 0; i < len; i++) {
            buf[i] = rx_buf[1 + i];
        }
    }
    return ret;
}

int adxl345_init(const struct device *dev)
{
    int ret;
    ret = adxl345_write_reg(dev, ADXL345_REG_POWER_CTL, 0x00);
    if (ret != 0) return -EIO;
    ret = adxl345_write_reg(dev, ADXL345_REG_POWER_CTL, 0x08);
    if (ret != 0) return -EIO;
    k_msleep(12);
    return 0;
}

int adxl345_read_xyz(const struct device *dev, int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    int ret = adxl345_read_burst(dev, ADXL345_REG_DATAX0, buf, 6);
    if (ret != 0) return -EIO;
    *x = (int16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    *y = (int16_t)(buf[2] | ((uint16_t)buf[3] << 8));
    *z = (int16_t)(buf[4] | ((uint16_t)buf[5] << 8));
    return 0;
}
