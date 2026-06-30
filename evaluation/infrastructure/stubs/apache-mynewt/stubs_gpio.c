#include "apache_mynewt.h"
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

static os_time_t g_time;

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

os_time_t os_time_get(void) { return g_time; }
void os_time_delay(os_time_t osticks) { g_time += osticks; }
int os_time_ms_to_ticks(uint32_t ms, os_time_t *out_ticks)
{
    if (out_ticks == 0) {
        return -1;
    }
    *out_ticks = ms;
    return 0;
}

void os_cputime_delay_usecs(uint32_t usecs)
{
    (void)usecs;
}

int hal_gpio_init_out(int pin, int val)
{
    gpio_clock_for_pin(pin);
    gpio_set_mode(pin, 1);
    hal_gpio_write(pin, val);
    return 0;
}

int hal_gpio_init_in(int pin, hal_gpio_pull_t pull)
{
    (void)pull;
    gpio_clock_for_pin(pin);
    gpio_set_mode(pin, 0);
    return 0;
}

void hal_gpio_write(int pin, int val)
{
    uint32_t base = gpio_base_for_pin(pin);
    uint32_t mask = gpio_mask_for_pin(pin);
    if (base == 0) {
        return;
    }
    GPIO_BSRR(base) = val ? mask : (mask << 16);
}

int hal_gpio_read(int pin)
{
    uint32_t base = gpio_base_for_pin(pin);
    uint32_t mask = gpio_mask_for_pin(pin);
    if (base == 0) {
        return 0;
    }
    return (GPIO_IDR(base) & mask) ? 1 : 0;
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
