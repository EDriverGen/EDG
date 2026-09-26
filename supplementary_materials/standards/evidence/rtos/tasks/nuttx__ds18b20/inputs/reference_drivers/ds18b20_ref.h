/*
 * DS18B20 1-Wire temperature sensor for NuttX (GPIO)
 */
#ifndef DS18B20_REF_H
#define DS18B20_REF_H

#include <nuttx/config.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DS18B20_CMD_SKIP_ROM       0xCC
#define DS18B20_CMD_CONVERT_T      0x44
#define DS18B20_CMD_READ_SCRATCH   0xBE
#define DS18B20_CONVERT_WAIT_MS    750

struct ds18b20_device
{
    int fd;
};

int ds18b20_init(struct ds18b20_device *dev, const char *gpio_path);
void ds18b20_deinit(struct ds18b20_device *dev);
int ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100);

#ifdef __cplusplus
}
#endif
#endif
