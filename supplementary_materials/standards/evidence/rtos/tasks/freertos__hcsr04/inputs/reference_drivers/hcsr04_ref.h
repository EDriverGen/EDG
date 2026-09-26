#ifndef __HCSR04_REF_H
#define __HCSR04_REF_H

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define HCSR04_SPEED_OF_SOUND_CM_US  0.0343f
#define HCSR04_MAX_DISTANCE_CM       400
#define HCSR04_TIMEOUT_US            25000

struct hcsr04_device {
    GPIO_TypeDef *trig_port;
    uint16_t trig_pin;
    GPIO_TypeDef *echo_port;
    uint16_t echo_pin;
};

/* Platform timing hooks must be provided by the board layer. */
extern void hcsr04_delay_us(uint32_t us);
extern uint32_t hcsr04_get_us_tick(void);

int hcsr04_init(struct hcsr04_device *dev, GPIO_TypeDef *trig_port, uint16_t trig_pin,
                GPIO_TypeDef *echo_port, uint16_t echo_pin);
int hcsr04_read_distance_mm(struct hcsr04_device *dev, int32_t *distance_mm);

#endif
