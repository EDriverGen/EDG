#ifndef DRIVERGEN_RTEMS_GPIO_H
#define DRIVERGEN_RTEMS_GPIO_H

#include <stdint.h>
#include <rtems.h>

typedef int rtems_gpio_pin;

typedef enum {
    RTEMS_GPIO_INPUT = 0,
    RTEMS_GPIO_OUTPUT = 1,
} rtems_gpio_direction;

rtems_status_code rtems_gpio_request_pin(rtems_gpio_pin pin, rtems_gpio_direction direction);
rtems_status_code rtems_gpio_release_pin(rtems_gpio_pin pin);
rtems_status_code rtems_gpio_set(rtems_gpio_pin pin);
rtems_status_code rtems_gpio_clear(rtems_gpio_pin pin);
rtems_status_code rtems_gpio_get_value(rtems_gpio_pin pin, int *value);

#endif
