/*
 * MAX31855 thermocouple driver for RT-Thread (SPI)
 */
#include "max31855_ref.h"

rt_err_t max31855_init(struct max31855_device *dev, const char *device_name)
{
    struct rt_spi_configuration cfg;
    if (dev == RT_NULL || device_name == RT_NULL)
        return -RT_EINVAL;

    dev->device_name = device_name;
    dev->spi = (struct rt_spi_device *)rt_device_find(device_name);
    if (dev->spi == RT_NULL)
        return -RT_ENOSYS;

    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.data_width = 8;
    cfg.max_hz = MAX31855_SPI_MAX_HZ;
    rt_spi_configure(dev->spi, &cfg);
    return RT_EOK;
}

rt_err_t max31855_read_raw(struct max31855_device *dev, rt_uint32_t *raw)
{
    rt_uint8_t buf[4];
    if (dev == RT_NULL || dev->spi == RT_NULL || raw == RT_NULL)
        return -RT_EINVAL;

    if (rt_spi_recv(dev->spi, buf, 4) != 4)
        return -RT_EIO;

    *raw = ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8)  |
           ((uint32_t)buf[3]);
    return RT_EOK;
}

rt_bool_t max31855_has_fault(rt_uint32_t raw)
{
    return (raw & MAX31855_FAULT_BIT) ? RT_TRUE : RT_FALSE;
}

rt_uint8_t max31855_get_fault(rt_uint32_t raw)
{
    return (rt_uint8_t)(raw & (MAX31855_FAULT_SCV | MAX31855_FAULT_SCG | MAX31855_FAULT_OC));
}

rt_err_t max31855_get_thermocouple_temp(rt_uint32_t raw, rt_int32_t *temp_mc)
{
    rt_int32_t val;
    if (temp_mc == RT_NULL) return -RT_EINVAL;
    if (raw & MAX31855_FAULT_BIT) return -RT_EINVAL;
    val = (rt_int32_t)(raw >> 18);
    if (val & 0x2000) val |= ~((rt_uint32_t)0x3FFF);
    *temp_mc = val * 250;
    return RT_EOK;
}

rt_err_t max31855_get_internal_temp(rt_uint32_t raw, rt_int32_t *temp_mc)
{
    rt_int32_t val;
    if (temp_mc == RT_NULL) return -RT_EINVAL;
    val = (rt_int32_t)((raw >> 4) & 0x0FFF);
    if (val & 0x0800) val |= ~((rt_uint32_t)0x0FFF);
    *temp_mc = (val * 625) / 10;
    return RT_EOK;
}

rt_err_t max31855_read_thermocouple(struct max31855_device *dev, rt_int32_t *temp_mc)
{
    rt_uint32_t raw;
    rt_err_t err = max31855_read_raw(dev, &raw);
    if (err != RT_EOK) return err;
    return max31855_get_thermocouple_temp(raw, temp_mc);
}

rt_err_t max31855_read_internal(struct max31855_device *dev, rt_int32_t *temp_mc)
{
    rt_uint32_t raw;
    rt_err_t err = max31855_read_raw(dev, &raw);
    if (err != RT_EOK) return err;
    return max31855_get_internal_temp(raw, temp_mc);
}
