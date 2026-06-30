#include "rtems.h"
#include "rtems/gpio.h"
#include "hw_uart.h"
#include <stdarg.h>
#include <stdio.h>

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018)
#define GPIOA_BASE  0x40010800U
#define GPIOB_BASE  0x40010C00U
#define GPIOC_BASE  0x40011000U
#define GPIO_CRL(b) (*(volatile uint32_t *)((b) + 0x00U))
#define GPIO_CRH(b) (*(volatile uint32_t *)((b) + 0x04U))
#define GPIO_IDR(b) (*(volatile uint32_t *)((b) + 0x08U))
#define GPIO_BSRR(b) (*(volatile uint32_t *)((b) + 0x10U))

static rtems_interval g_ticks;

static uint32_t gpio_base_for_pin(int pin)
{
    int port = pin / 16;
    if (port == 0) return GPIOA_BASE;
    if (port == 1) return GPIOB_BASE;
    if (port == 2) return GPIOC_BASE;
    return 0;
}

static uint32_t gpio_mask_for_pin(int pin)
{
    return 1U << (pin % 16);
}

static void gpio_clock_for_pin(int pin)
{
    int port = pin / 16;
    if (port == 0) RCC_APB2ENR |= (1U << 2);
    else if (port == 1) RCC_APB2ENR |= (1U << 3);
    else if (port == 2) RCC_APB2ENR |= (1U << 4);
}

static void gpio_set_mode(int pin, int output)
{
    uint32_t base = gpio_base_for_pin(pin);
    int idx = pin % 16;
    int shift = (idx < 8 ? idx : idx - 8) * 4;
    volatile uint32_t *reg = idx < 8 ? &GPIO_CRL(base) : &GPIO_CRH(base);
    uint32_t v;
    if (base == 0) {
        return;
    }
    v = *reg;
    v &= ~(0xFU << shift);
    v |= ((output ? 0x1U : 0x4U) << shift);
    *reg = v;
}

rtems_status_code rtems_gpio_request_pin(rtems_gpio_pin pin, rtems_gpio_direction direction)
{
    gpio_clock_for_pin(pin);
    gpio_set_mode(pin, direction == RTEMS_GPIO_OUTPUT);
    return RTEMS_SUCCESSFUL;
}

rtems_status_code rtems_gpio_release_pin(rtems_gpio_pin pin)
{
    (void)pin;
    return RTEMS_SUCCESSFUL;
}

rtems_status_code rtems_gpio_set(rtems_gpio_pin pin)
{
    uint32_t base = gpio_base_for_pin(pin);
    uint32_t mask = gpio_mask_for_pin(pin);
    if (base == 0) {
        return RTEMS_INVALID_ID;
    }
    GPIO_BSRR(base) = mask;
    return RTEMS_SUCCESSFUL;
}

rtems_status_code rtems_gpio_clear(rtems_gpio_pin pin)
{
    uint32_t base = gpio_base_for_pin(pin);
    uint32_t mask = gpio_mask_for_pin(pin);
    if (base == 0) {
        return RTEMS_INVALID_ID;
    }
    GPIO_BSRR(base) = mask << 16;
    return RTEMS_SUCCESSFUL;
}

rtems_status_code rtems_gpio_get_value(rtems_gpio_pin pin, int *value)
{
    uint32_t base = gpio_base_for_pin(pin);
    uint32_t mask = gpio_mask_for_pin(pin);
    if (base == 0 || value == 0) {
        return RTEMS_INVALID_ID;
    }
    *value = (GPIO_IDR(base) & mask) ? 1 : 0;
    return RTEMS_SUCCESSFUL;
}

rtems_status_code rtems_task_wake_after(rtems_interval ticks)
{
    g_ticks += ticks;
    return RTEMS_SUCCESSFUL;
}

rtems_interval rtems_clock_get_ticks_per_second(void)
{
    return 1000;
}

int printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) {
        hw_uart2_putc(buf[i]);
    }
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
