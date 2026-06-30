/*
 * MAX31855 thermocouple driver for RT-Thread (SPI, read-only)
 */
#ifndef DRIVERS_INCLUDE_MAX31855_H_
#define DRIVERS_INCLUDE_MAX31855_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX31855_SPI_MAX_HZ  5000000

#define MAX31855_FAULT_BIT   (1u << 16)
#define MAX31855_FAULT_SCV   (1u << 2)
#define MAX31855_FAULT_SCG   (1u << 1)
#define MAX31855_FAULT_OC    (1u << 0)

struct max31855_device
{
    struct rt_spi_device *spi;
    const char *device_name;
};

rt_err_t max31855_init(struct max31855_device *dev, const char *device_name);
rt_err_t max31855_read_raw(struct max31855_device *dev, rt_uint32_t *raw);
rt_bool_t max31855_has_fault(rt_uint32_t raw);
rt_uint8_t max31855_get_fault(rt_uint32_t raw);
rt_err_t max31855_get_thermocouple_temp(rt_uint32_t raw, rt_int32_t *temp_mc);
rt_err_t max31855_get_internal_temp(rt_uint32_t raw, rt_int32_t *temp_mc);
rt_err_t max31855_read_thermocouple(struct max31855_device *dev, rt_int32_t *temp_mc);
rt_err_t max31855_read_internal(struct max31855_device *dev, rt_int32_t *temp_mc);

#ifdef __cplusplus
}
#endif
#endif
