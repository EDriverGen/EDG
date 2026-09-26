/*
 * DS18B20 1-Wire temperature sensor for RT-Thread (GPIO)
 */
#ifndef DRIVERS_INCLUDE_DS18B20_H_
#define DRIVERS_INCLUDE_DS18B20_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DS18B20_CMD_SKIP_ROM       0xCC
#define DS18B20_CMD_CONVERT_T      0x44
#define DS18B20_CMD_READ_SCRATCH   0xBE
#define DS18B20_CONVERT_WAIT_MS    750

struct ds18b20_device
{
    rt_base_t data_pin;
};

rt_err_t ds18b20_init(struct ds18b20_device *dev, rt_base_t data_pin);
rt_err_t ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100);

/* Pure decoder — host-testable. Validates CRC8 over the first 8 scratchpad
 * bytes against byte 9, then extracts the raw 16-bit temperature
 * (0.0625 °C/LSB → temp_x16) and decodes the configuration register's
 * resolution bits (9..12). Returns 0 on CRC pass, -1 on CRC fail. Outputs
 * are written even when CRC fails so callers can inspect raw values. */
int ds18b20_decode_scratchpad(const unsigned char scratchpad[9],
                              int *temp_x16,
                              unsigned char *resolution_bits,
                              int *crc_ok);

#ifdef __cplusplus
}
#endif
#endif
