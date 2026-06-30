/*
 * ADXL345 accelerometer driver for RT-Thread (SPI)
 */
#include "adxl345_ref.h"

rt_err_t adxl345_write_reg(struct adxl345_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2];
    if (dev == RT_NULL || dev->spi == RT_NULL) return -RT_EINVAL;
    tx[0] = reg & 0x3F;  /* write bit = 0 */
    tx[1] = val;
    if (rt_spi_send(dev->spi, tx, 2) != 2) return -RT_EIO;
    return RT_EOK;
}

rt_err_t adxl345_read_reg(struct adxl345_device *dev, uint8_t reg, uint8_t *val)
{
    uint8_t tx[1], rx[1];
    if (dev == RT_NULL || dev->spi == RT_NULL || val == RT_NULL) return -RT_EINVAL;
    tx[0] = ADXL345_SPI_READ | (reg & 0x3F);
    if (rt_spi_send_then_recv(dev->spi, tx, 1, rx, 1) != RT_EOK) return -RT_EIO;
    *val = rx[0];
    return RT_EOK;
}

rt_err_t adxl345_init(struct adxl345_device *dev, const char *device_name, uint8_t range)
{
    struct rt_spi_configuration cfg;
    uint8_t id;
    if (dev == RT_NULL || device_name == RT_NULL) return -RT_EINVAL;
    dev->device_name = device_name;
    dev->spi = (struct rt_spi_device *)rt_device_find(device_name);
    if (dev->spi == RT_NULL) return -RT_ENOSYS;
    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_3 | RT_SPI_MSB;
    cfg.data_width = 8;
    cfg.max_hz = ADXL345_SPI_MAX_HZ;
    rt_spi_configure(dev->spi, &cfg);

    if (adxl345_read_id(dev, &id) != RT_EOK || id != ADXL345_DEVID)
        return -RT_EIO;
    adxl345_write_reg(dev, ADXL345_REG_DATA_FMT, ADXL345_FULL_RES_BIT | (range & 0x03));
    adxl345_write_reg(dev, ADXL345_REG_BW_RATE, 0x0A);  /* 100 Hz */
    adxl345_write_reg(dev, ADXL345_REG_POWER_CTL, ADXL345_MEASURE_BIT);
    return RT_EOK;
}

rt_err_t adxl345_read_id(struct adxl345_device *dev, uint8_t *id)
{
    return adxl345_read_reg(dev, ADXL345_REG_DEVID, id);
}

rt_err_t adxl345_read_accel(struct adxl345_device *dev, struct adxl345_accel *accel)
{
    uint8_t tx[1], rx[6];
    if (dev == RT_NULL || dev->spi == RT_NULL || accel == RT_NULL) return -RT_EINVAL;
    tx[0] = ADXL345_SPI_READ | ADXL345_SPI_MB | ADXL345_REG_DATAX0;
    if (rt_spi_send_then_recv(dev->spi, tx, 1, rx, 6) != RT_EOK) return -RT_EIO;
    accel->x = (int16_t)(rx[0] | ((uint16_t)rx[1] << 8));
    accel->y = (int16_t)(rx[2] | ((uint16_t)rx[3] << 8));
    accel->z = (int16_t)(rx[4] | ((uint16_t)rx[5] << 8));
    return RT_EOK;
}

rt_err_t adxl345_read_accel_mg(struct adxl345_device *dev, int32_t *x_mg, int32_t *y_mg, int32_t *z_mg)
{
    struct adxl345_accel a;
    rt_err_t err = adxl345_read_accel(dev, &a);
    if (err != RT_EOK) return err;
    *x_mg = (int32_t)a.x * ADXL345_SCALE_MG;
    *y_mg = (int32_t)a.y * ADXL345_SCALE_MG;
    *z_mg = (int32_t)a.z * ADXL345_SCALE_MG;
    return RT_EOK;
}
