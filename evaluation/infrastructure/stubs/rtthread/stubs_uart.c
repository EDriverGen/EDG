/* Functional RT-Thread UART stubs for the evaluation harness.
 *
 * Routes `rt_device_write` / `rt_device_read` on the "fake serial" device
 * through the bare-metal STM32 USART1 helpers in `hw_uart_bus.h`. That
 * lets UART-based drivers exchange real bytes with the
 * Python oracle (stm32_usart_hw_slave.py) in Renode.
 *
 * `rt_device_find` is a macro that expands to `rt_i2c_bus_device_find`
 * (see rtthread.h line 207), so we provide that symbol and reinterpret
 * the pointer as the generic rt_device_t used by serial drivers.
 *
 * Configuration (rt_device_control with RT_DEVICE_CTRL_CONFIG) and open
 * are captured as no-ops because the Python USART model doesn't model
 * baud-rate negotiation — it just answers fixed command/response pairs.
 */

#include "rtthread.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- Fake serial device (reused as the opaque rt_device_t handle) ---- */
static struct rt_i2c_bus_device fake_serial;

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *name) {
    (void)name;
    return &fake_serial;
}

/* ---- Generic device file-ops, routed to USART1 ---- */
rt_err_t rt_device_open(rt_device_t dev, rt_uint16_t oflag) { (void)dev; (void)oflag; return RT_EOK; }
rt_err_t rt_device_close(rt_device_t dev) { (void)dev; return RT_EOK; }
rt_err_t rt_device_control(rt_device_t dev, int cmd, void *arg) { (void)dev; (void)cmd; (void)arg; return RT_EOK; }

rt_size_t rt_device_write(rt_device_t dev, rt_off_t pos,
                          const void *buf, rt_size_t size) {
    (void)dev; (void)pos;
    const uint8_t *p = (const uint8_t *)buf;
    for (rt_size_t i = 0; i < size; i++) hw_uart_bus_write_byte(p[i]);
    return size;
}

rt_size_t rt_device_read(rt_device_t dev, rt_off_t pos,
                         void *buf, rt_size_t size) {
    (void)dev; (void)pos;
    uint8_t *p = (uint8_t *)buf;
    for (rt_size_t i = 0; i < size; i++) {
        if (hw_uart_bus_read_byte(&p[i]) != 0) return i;   /* timeout */
    }
    return size;
}

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

void rt_pin_mode(rt_base_t pin, rt_base_t mode) { (void)pin; (void)mode; }
void rt_pin_write(rt_base_t pin, rt_base_t value) { (void)pin; (void)value; }
rt_base_t rt_pin_read(rt_base_t pin) { (void)pin; return 0; }

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
