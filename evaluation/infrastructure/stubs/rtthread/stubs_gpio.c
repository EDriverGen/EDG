/* RT-Thread GPIO stubs for the evaluation harness.
 *
 * The stubs map RT-Thread pin numbers to STM32 GPIO registers watched by the
 * Renode pulse injector. GPIO clocks are enabled lazily on first use. */

#include "rtthread.h"
#include "hw_uart.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- STM32F103 GPIO registers ---- */
#define RCC_APB2ENR  (*(volatile uint32_t*)0x40021018)
#define GPIOA_BASE   0x40010800
#define GPIOB_BASE   0x40010C00
#define GPIOC_BASE   0x40011000
#define GPIO_CRL(b)  (*(volatile uint32_t*)((b) + 0x00))
#define GPIO_CRH(b)  (*(volatile uint32_t*)((b) + 0x04))
#define GPIO_IDR(b)  (*(volatile uint32_t*)((b) + 0x08))
#define GPIO_ODR(b)  (*(volatile uint32_t*)((b) + 0x0C))
#define GPIO_BSRR(b) (*(volatile uint32_t*)((b) + 0x10))

/* RCC_APB2ENR bits: IOPAEN=2, IOPBEN=3, IOPCEN=4 */
#define IOPAEN (1U << 2)
#define IOPBEN (1U << 3)
#define IOPCEN (1U << 4)

static uint32_t _port_base_for(rt_base_t port_idx) {
    switch (port_idx) {
        case 0: return GPIOA_BASE;
        case 1: return GPIOB_BASE;
        case 2: return GPIOC_BASE;
        default: return 0;
    }
}

static void _ensure_port_clock(rt_base_t port_idx) {
    switch (port_idx) {
        case 0: RCC_APB2ENR |= IOPAEN; break;
        case 1: RCC_APB2ENR |= IOPBEN; break;
        case 2: RCC_APB2ENR |= IOPCEN; break;
        default: break;
    }
}

static void _set_pin_mode(uint32_t base, int pin, rt_base_t mode) {
    /* CRL for pins 0-7, CRH for pins 8-15. Each pin uses a 4-bit nibble.
     * mode=PIN_MODE_OUTPUT (1) -> CNF=00 (push-pull), MODE=01 (10 MHz) -> 0x1
     * mode=PIN_MODE_INPUT  (0) -> CNF=01 (floating),   MODE=00        -> 0x4 */
    const int shift = (pin < 8 ? pin : pin - 8) * 4;
    volatile uint32_t *reg = (pin < 8)
        ? (volatile uint32_t*)(base + 0x00)   /* CRL */
        : (volatile uint32_t*)(base + 0x04);  /* CRH */
    uint32_t v = *reg;
    v &= ~(0xFU << shift);
    v |=  ((mode == 1 ? 0x1U : 0x4U) << shift);
    *reg = v;
}

/* ---- rt_pin_* (port*16 + pin encoding; see rtthread.h GET_PIN macro) ---- */
void rt_pin_mode(rt_base_t pin, rt_base_t mode) {
    rt_base_t port = pin / 16;
    int idx = (int)(pin % 16);
    uint32_t base = _port_base_for(port);
    if (base == 0) return;
    _ensure_port_clock(port);
    _set_pin_mode(base, idx, mode);
}

void rt_pin_write(rt_base_t pin, rt_base_t value) {
    rt_base_t port = pin / 16;
    int idx = (int)(pin % 16);
    uint32_t base = _port_base_for(port);
    if (base == 0) return;
    /* BSRR: low half = BSx (set), high half = BRx (reset). */
    if (value) GPIO_BSRR(base) = (1U << idx);
    else       GPIO_BSRR(base) = (1U << (idx + 16));
}

rt_base_t rt_pin_read(rt_base_t pin) {
    rt_base_t port = pin / 16;
    int idx = (int)(pin % 16);
    uint32_t base = _port_base_for(port);
    if (base == 0) return 0;
    return (GPIO_IDR(base) >> idx) & 0x1U;
}

/* ---- rt_device_find macro target (drivers rarely use this on GPIO buses) ---- */
static struct rt_i2c_bus_device fake_gpio_bus;

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *name) {
    (void)name;
    return &fake_gpio_bus;
}

rt_err_t rt_device_open(rt_device_t dev, rt_uint16_t oflag) { (void)dev; (void)oflag; return RT_EOK; }
rt_err_t rt_device_close(rt_device_t dev) { (void)dev; return RT_EOK; }
rt_err_t rt_device_control(rt_device_t dev, int cmd, void *arg) { (void)dev; (void)cmd; (void)arg; return RT_EOK; }
rt_size_t rt_device_read(rt_device_t dev, rt_off_t pos, void *buf, rt_size_t size) { (void)dev; (void)pos; (void)buf; return size; }
rt_size_t rt_device_write(rt_device_t dev, rt_off_t pos, const void *buf, rt_size_t size) { (void)dev; (void)pos; (void)buf; return size; }

/* ---- Kernel / thread / mutex / sem shims (shared with stubs_i2c.c) ---- */
rt_err_t rt_thread_delay(rt_tick_t tick) { (void)tick; return RT_EOK; }
rt_err_t rt_thread_delay_until(rt_tick_t *tick, rt_tick_t inc_tick) {
    if (tick) *tick += inc_tick;
    return RT_EOK;
}
rt_tick_t rt_tick_get_delta(rt_tick_t since) { (void)since; return 0; }
rt_err_t rt_thread_mdelay(rt_int32_t ms) {
    if (ms > 0) {
        for (volatile uint32_t i = 0; i < (uint32_t)ms * 800; i++) {}
    }
    return RT_EOK;
}
rt_tick_t rt_tick_get(void) { static rt_tick_t t = 0; return t += 23; }
rt_uint32_t rt_tick_from_millisecond(rt_uint32_t ms) { return ms; }

static int _rt_mtx_dummy;
rt_mutex_t rt_mutex_create(const char *n, rt_uint8_t f) { (void)n; (void)f; return (rt_mutex_t)&_rt_mtx_dummy; }
rt_err_t rt_mutex_take(rt_mutex_t m, rt_int32_t t) { (void)m; (void)t; return RT_EOK; }
rt_err_t rt_mutex_release(rt_mutex_t m) { (void)m; return RT_EOK; }
rt_err_t rt_mutex_delete(rt_mutex_t m) { (void)m; return RT_EOK; }

rt_sem_t rt_sem_create(const char *n, rt_uint32_t v, rt_uint8_t f) { (void)n; (void)v; (void)f; return (rt_sem_t)&_rt_mtx_dummy; }
rt_err_t rt_sem_take(rt_sem_t s, rt_int32_t t) { (void)s; (void)t; return RT_EOK; }
rt_err_t rt_sem_release(rt_sem_t s) { (void)s; return RT_EOK; }
rt_err_t rt_sem_delete(rt_sem_t s) { (void)s; return RT_EOK; }

static int _rt_thread_dummy;
rt_thread_t rt_thread_create(const char *n, void (*e)(void*),
                             void *p, rt_uint32_t ss,
                             rt_uint8_t pr, rt_uint32_t tk) {
    (void)n; (void)e; (void)p; (void)ss; (void)pr; (void)tk;
    return (rt_thread_t)&_rt_thread_dummy;
}
rt_err_t rt_thread_startup(rt_thread_t t) { (void)t; return RT_EOK; }

void *rt_realloc(void *ptr, rt_size_t size) { (void)ptr; (void)size; return 0; }
rt_base_t rt_hw_interrupt_disable(void) { return 0; }
void rt_hw_interrupt_enable(rt_base_t level) { (void)level; }

int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
