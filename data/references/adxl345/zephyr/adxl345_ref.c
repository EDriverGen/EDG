/*
 * ADXL345 accelerometer driver for Zephyr (SPI)
 */
#include "adxl345_ref.h"

int adxl345_write_reg(struct adxl345_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg & 0x3F, val };
    struct spi_buf buf = { .buf = tx, .len = 2 };
    struct spi_buf_set set = { .buffers = &buf, .count = 1 };
    if (!dev || !dev->spi_dev) return -EINVAL;
    return spi_write(dev->spi_dev, &dev->spi_cfg, &set);
}

int adxl345_read_reg(struct adxl345_device *dev, uint8_t reg, uint8_t *val)
{
    uint8_t tx[2] = { ADXL345_SPI_READ | (reg & 0x3F), 0 };
    uint8_t rx[2] = {0};
    struct spi_buf tx_buf = { .buf = tx, .len = 2 };
    struct spi_buf rx_buf = { .buf = rx, .len = 2 };
    struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
    struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };
    if (!dev || !dev->spi_dev || !val) return -EINVAL;
    int ret = spi_transceive(dev->spi_dev, &dev->spi_cfg, &tx_set, &rx_set);
    if (ret < 0) return ret;
    *val = rx[1];
    return 0;
}

int adxl345_init(struct adxl345_device *dev, const struct device *spi,
                 const struct gpio_dt_spec *cs_gpio, uint8_t range)
{
    uint8_t id;
    if (!dev || !spi) return -EINVAL;
    if (!device_is_ready(spi)) return -ENODEV;
    dev->spi_dev = spi;
    dev->spi_cfg.frequency = ADXL345_SPI_MAX_HZ;
    dev->spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA;
    dev->spi_cfg.slave = 0;
    if (cs_gpio) { dev->cs_ctrl.gpio = *cs_gpio; dev->cs_ctrl.delay = 0;
                   dev->spi_cfg.cs = &dev->cs_ctrl; }
    else dev->spi_cfg.cs = NULL;
    if (adxl345_read_id(dev, &id) || id != ADXL345_DEVID) return -EIO;
    adxl345_write_reg(dev, ADXL345_REG_DATA_FMT, ADXL345_FULL_RES_BIT | (range & 0x03));
    adxl345_write_reg(dev, ADXL345_REG_BW_RATE, 0x0A);
    adxl345_write_reg(dev, ADXL345_REG_POWER_CTL, ADXL345_MEASURE_BIT);
    return 0;
}

int adxl345_read_id(struct adxl345_device *dev, uint8_t *id)
{ return adxl345_read_reg(dev, ADXL345_REG_DEVID, id); }

int adxl345_read_accel(struct adxl345_device *dev, struct adxl345_accel *accel)
{
    uint8_t tx[7] = {0}, rx[7] = {0};
    struct spi_buf tx_buf = { .buf = tx, .len = 7 };
    struct spi_buf rx_buf = { .buf = rx, .len = 7 };
    struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
    struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };
    if (!dev || !dev->spi_dev || !accel) return -EINVAL;
    tx[0] = ADXL345_SPI_READ | ADXL345_SPI_MB | ADXL345_REG_DATAX0;
    int ret = spi_transceive(dev->spi_dev, &dev->spi_cfg, &tx_set, &rx_set);
    if (ret < 0) return ret;
    accel->x = (int16_t)(rx[1] | ((uint16_t)rx[2] << 8));
    accel->y = (int16_t)(rx[3] | ((uint16_t)rx[4] << 8));
    accel->z = (int16_t)(rx[5] | ((uint16_t)rx[6] << 8));
    return 0;
}

int adxl345_read_accel_mg(struct adxl345_device *dev, int32_t *x_mg, int32_t *y_mg, int32_t *z_mg)
{
    struct adxl345_accel a;
    int ret = adxl345_read_accel(dev, &a);
    if (ret) return ret;
    *x_mg = (int32_t)a.x * ADXL345_SCALE_MG;
    *y_mg = (int32_t)a.y * ADXL345_SCALE_MG;
    *z_mg = (int32_t)a.z * ADXL345_SCALE_MG;
    return 0;
}
