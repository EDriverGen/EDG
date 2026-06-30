/*
 * MAX31855 thermocouple driver for Zephyr (SPI)
 */
#ifndef MAX31855_REF_H
#define MAX31855_REF_H

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX31855_FAULT_BIT   (1u << 16)
#define MAX31855_FAULT_SCV   (1u << 2)
#define MAX31855_FAULT_SCG   (1u << 1)
#define MAX31855_FAULT_OC    (1u << 0)

struct max31855_device
{
    const struct device *spi_dev;
    struct spi_config spi_cfg;
    struct spi_cs_control cs_ctrl;
};

int max31855_init(struct max31855_device *dev, const struct device *spi,
                  const struct gpio_dt_spec *cs_gpio);
int max31855_read_raw(struct max31855_device *dev, uint32_t *raw);
int max31855_has_fault(uint32_t raw);
uint8_t max31855_get_fault(uint32_t raw);
int max31855_get_thermocouple_temp(uint32_t raw, int32_t *temp_mc);
int max31855_get_internal_temp(uint32_t raw, int32_t *temp_mc);
int max31855_read_thermocouple(struct max31855_device *dev, int32_t *temp_mc);
int max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc);

#ifdef __cplusplus
}
#endif
#endif
