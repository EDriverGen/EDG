/*
 * DS18B20 1-Wire temperature sensor for ThreadX (HAL-agnostic)
 */
#ifndef DS18B20_REF_H
#define DS18B20_REF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DS18B20_CMD_SKIP_ROM       0xCC
#define DS18B20_CMD_CONVERT_T      0x44
#define DS18B20_CMD_READ_SCRATCH   0xBE
#define DS18B20_CONVERT_WAIT_MS    750

struct ds18b20_ow_ops
{
    void (*set_output)(void *ctx);
    void (*set_input)(void *ctx);
    void (*write)(void *ctx, int val);
    int  (*read)(void *ctx);
    void (*delay_us)(uint32_t us);
    void (*delay_ms)(uint32_t ms);
};

struct ds18b20_device
{
    const struct ds18b20_ow_ops *ops;
    void *ctx;
};

int ds18b20_init(struct ds18b20_device *dev, const struct ds18b20_ow_ops *ops, void *ctx);
int ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100);

#ifdef __cplusplus
}
#endif

/* Alias for adapter compatibility */
#define ds18b20_gpio_ops ds18b20_ow_ops

#endif