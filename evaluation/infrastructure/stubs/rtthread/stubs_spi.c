/* Functional RT-Thread SPI stubs for the evaluation harness.
 *
 * Routes `rt_spi_send` / `rt_spi_recv` / `rt_spi_send_then_recv` /
 * `rt_spi_transfer` through the bare-metal STM32 SPI1 helpers in
 * `hw_spi.h`. Each call issues one CS-bracketed transaction so the
 * Python slave (stm32_spi_hw_slave.py) sees a clean TXN_RESET before
 * every frame. `rt_spi_send_then_recv` keeps CS asserted across the
 * tx+rx halves (required by chips like ADXL345).
 *
 * Kernel/thread/mutex/sem/pin primitives remain dummy no-ops — drivers
 * rely on them only for bookkeeping; anything that would need real
 * scheduling or IRQs is out of scope for L1-L4.
 *
 * Note on rt_device_find: rtthread.h defines it as a macro that calls
 * `rt_i2c_bus_device_find`, so we reuse that symbol and just return a
 * non-NULL pointer whose memory is reinterpreted by the driver as a
 * `struct rt_spi_device *`. The fake bus has just enough storage to
 * keep `rt_spi_configure` writes from corrupting anything else.
 */

#include "rtthread.h"
#include "hw_spi.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "hw_uart.h"

/* ---- Fake SPI device (storage for the pointer returned by rt_device_find) ---- */
static struct rt_spi_device fake_spi;

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *name) {
    (void)name;
    /* rtthread.h macro: rt_device_find(n) -> (rt_device_t)rt_i2c_bus_device_find(n).
     * We return the address of fake_spi cast back, so the driver's cast to
     * `struct rt_spi_device *` yields a valid, writable pointer. */
    return (struct rt_i2c_bus_device *)&fake_spi;
}

/* ---- rt_spi_* routed through hw_spi.h ---- */
rt_err_t rt_spi_configure(struct rt_spi_device *device,
                          struct rt_spi_configuration *cfg) {
    if (device && cfg) device->config = *cfg;
    return RT_EOK;
}

rt_err_t rt_spi_send(struct rt_spi_device *device,
                     const void *send_buf, rt_size_t length) {
    (void)device;
    const uint8_t *tx = (const uint8_t *)send_buf;
    hw_spi1_cs_lo();
    for (rt_size_t i = 0; i < length; i++) (void)hw_spi1_xfer_byte(tx[i]);
    hw_spi1_cs_hi();
    return (rt_err_t)length;
}

rt_err_t rt_spi_recv(struct rt_spi_device *device,
                     void *recv_buf, rt_size_t length) {
    (void)device;
    uint8_t *rx = (uint8_t *)recv_buf;
    hw_spi1_cs_lo();
    for (rt_size_t i = 0; i < length; i++) rx[i] = hw_spi1_xfer_byte(0x00);
    hw_spi1_cs_hi();
    return (rt_err_t)length;
}

rt_err_t rt_spi_send_then_recv(struct rt_spi_device *device,
                               const void *send_buf, rt_size_t send_length,
                               void *recv_buf, rt_size_t recv_length) {
    (void)device;
    const uint8_t *tx = (const uint8_t *)send_buf;
    uint8_t *rx = (uint8_t *)recv_buf;
    hw_spi1_cs_lo();
    for (rt_size_t i = 0; i < send_length; i++) (void)hw_spi1_xfer_byte(tx[i]);
    for (rt_size_t i = 0; i < recv_length; i++) rx[i] = hw_spi1_xfer_byte(0x00);
    hw_spi1_cs_hi();
    return RT_EOK;
}

rt_err_t rt_spi_transfer(struct rt_spi_device *device,
                         const void *send_buf, void *recv_buf,
                         rt_size_t length) {
    (void)device;
    const uint8_t *tx = (const uint8_t *)send_buf;
    uint8_t *rx = (uint8_t *)recv_buf;
    hw_spi1_cs_lo();
    for (rt_size_t i = 0; i < length; i++) {
        uint8_t tb = tx ? tx[i] : 0x00;
        uint8_t rb = hw_spi1_xfer_byte(tb);
        if (rx) rx[i] = rb;
    }
    hw_spi1_cs_hi();
    return (rt_err_t)length;
}

rt_err_t rt_spi_send_then_send(struct rt_spi_device *device,
                               const void *buf1, rt_size_t len1,
                               const void *buf2, rt_size_t len2) {
    (void)device;
    const uint8_t *b1 = (const uint8_t *)buf1;
    const uint8_t *b2 = (const uint8_t *)buf2;
    hw_spi1_cs_lo();
    for (rt_size_t i = 0; i < len1; i++) (void)hw_spi1_xfer_byte(b1[i]);
    for (rt_size_t i = 0; i < len2; i++) (void)hw_spi1_xfer_byte(b2[i]);
    hw_spi1_cs_hi();
    return RT_EOK;
}

struct rt_spi_device *rt_spi_bus_attach_device(struct rt_spi_device *device,
                                               const char *name,
                                               const char *bus_name,
                                               void *user_data) {
    (void)name; (void)bus_name; (void)user_data;
    return device ? device : &fake_spi;
}

/* ---- Generic device file-ops (drivers rarely use these on SPI buses) ---- */
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
