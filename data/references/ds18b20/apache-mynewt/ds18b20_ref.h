#ifndef DS18B20_APACHE_MYNEWT_REF_H
#define DS18B20_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include "hal/hal_gpio.h"
#include <stdint.h>

#define DS18B20_CMD_SKIP_ROM     0xCCU
#define DS18B20_CMD_CONVERT_T    0x44U
#define DS18B20_CMD_READ_SCRATCH 0xBEU
#define DS18B20_CONVERT_WAIT_MS  750U

struct ds18b20_device {
    int data_pin;
};

int ds18b20_init(struct ds18b20_device *dev, int data_pin);
int ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100);
int ds18b20_decode_scratchpad(const unsigned char scratchpad[9],
                              int *temp_x16,
                              unsigned char *resolution_bits,
                              int *crc_ok);

#endif
