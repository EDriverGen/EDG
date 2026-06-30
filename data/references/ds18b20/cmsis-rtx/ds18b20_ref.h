#ifndef DS18B20_CMSIS_RTX_REF_H
#define DS18B20_CMSIS_RTX_REF_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define DS18B20_CMD_SKIP_ROM     0xCCU
#define DS18B20_CMD_CONVERT_T    0x44U
#define DS18B20_CMD_READ_SCRATCH 0xBEU
#define DS18B20_CONVERT_WAIT_MS  750U

struct ds18b20_device {
    GPIO_TypeDef *port;
    uint16_t pin;
};

int ds18b20_init(struct ds18b20_device *dev, GPIO_TypeDef *port, uint16_t pin);
int ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100);
int ds18b20_decode_scratchpad(const unsigned char scratchpad[9],
                              int *temp_x16,
                              unsigned char *resolution_bits,
                              int *crc_ok);

#endif
