/*
 * DS18B20 1-Wire temperature sensor for FreeRTOS + STM32 HAL
 */
#ifndef DS18B20_REF_H
#define DS18B20_REF_H

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS18B20_CMD_SKIP_ROM       0xCC
#define DS18B20_CMD_CONVERT_T      0x44
#define DS18B20_CMD_READ_SCRATCH   0xBE
#define DS18B20_CONVERT_WAIT_MS    750

struct ds18b20_device
{
    GPIO_TypeDef *port;
    uint16_t pin;
};

int ds18b20_init(struct ds18b20_device *dev, GPIO_TypeDef *port, uint16_t pin);
int ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100);
extern void ds18b20_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif
#endif
