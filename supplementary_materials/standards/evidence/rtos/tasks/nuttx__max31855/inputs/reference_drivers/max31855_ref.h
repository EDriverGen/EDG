#ifndef __DRIVERS_MAX31855_H
#define __DRIVERS_MAX31855_H

#include <nuttx/config.h>
#include <nuttx/spi/spi.h>
#include <stdint.h>

#define MAX31855_FAULT_BIT   (1u << 16)
#define MAX31855_FAULT_SCV   (1u << 2)
#define MAX31855_FAULT_SCG   (1u << 1)
#define MAX31855_FAULT_OC    (1u << 0)

struct max31855_device { FAR struct spi_dev_s *spi; uint32_t devid; };

int max31855_init(FAR struct max31855_device *dev, FAR struct spi_dev_s *spi, uint32_t devid);
int max31855_read_raw(FAR struct max31855_device *dev, FAR uint32_t *raw);
int max31855_has_fault(uint32_t raw);
uint8_t max31855_get_fault(uint32_t raw);
int max31855_get_thermocouple_temp(uint32_t raw, FAR int32_t *temp_mc);
int max31855_get_internal_temp(uint32_t raw, FAR int32_t *temp_mc);
int max31855_read_thermocouple(FAR struct max31855_device *dev, FAR int32_t *temp_mc);
int max31855_read_internal(FAR struct max31855_device *dev, FAR int32_t *temp_mc);

#endif
