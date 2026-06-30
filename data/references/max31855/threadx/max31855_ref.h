#ifndef MAX31855_REF_H
#define MAX31855_REF_H

#include <stdint.h>

#define MAX31855_FAULT_BIT   (1u << 16)
#define MAX31855_FAULT_SCV   (1u << 2)
#define MAX31855_FAULT_SCG   (1u << 1)
#define MAX31855_FAULT_OC    (1u << 0)

struct max31855_spi_ops {
    void (*cs_select)(void *ctx);
    void (*cs_deselect)(void *ctx);
    int (*spi_recv)(void *ctx, uint8_t *buf, uint32_t len);
};

struct max31855_device {
    const struct max31855_spi_ops *ops;
    void *ctx;
};

int max31855_init(struct max31855_device *dev, const struct max31855_spi_ops *ops, void *ctx);
int max31855_read_raw(struct max31855_device *dev, uint32_t *raw);
int max31855_has_fault(uint32_t raw);
uint8_t max31855_get_fault(uint32_t raw);
int max31855_get_thermocouple_temp(uint32_t raw, int32_t *temp_mc);
int max31855_get_internal_temp(uint32_t raw, int32_t *temp_mc);
int max31855_read_thermocouple(struct max31855_device *dev, int32_t *temp_mc);
int max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc);

#endif
