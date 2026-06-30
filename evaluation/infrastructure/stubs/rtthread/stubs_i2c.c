/* Functional RT-Thread I2C stubs for the evaluation harness.
 *
 * `rt_i2c_transfer` routes every message to `hw_i2c_read`/`hw_i2c_write`
 * in hw_i2c.h so the driver really drives STM32 I2C1 registers in Renode.
 * That lets the Python slave (i2c_register_slave.py) see writes/reads and
 * the trace recorder log them, which is a prerequisite for L3/L4.
 *
 * Kernel/thread/mutex/sem primitives remain dummy (no-ops) — drivers
 * rely on them only for side-effect-free bookkeeping. Anything that
 * would require real scheduling or timing is out of scope for L1-L4.
 *
 * Structure: bus block (routes rt_i2c_transfer → hw_i2c_*) is kept
 * first so driver sources see its definitions before the kernel block.
 *
 *  * file becomes the canonical copy.
 */

/* ---- I2C bus stubs (real STM32 I2C1 via hw_i2c.h) ---- */
#include "rtthread.h"
#include "hw_i2c.h"
#include <string.h>

static struct rt_i2c_bus_device fake_bus;

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *name) {
    (void)name;
    return &fake_bus;
}

rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg msgs[], rt_uint32_t num) {
    (void)bus;
    for (rt_uint32_t i = 0; i < num; i++) {
        if (msgs[i].flags & RT_I2C_RD) {
            if (hw_i2c_read(0, msgs[i].addr, msgs[i].buf, msgs[i].len) != 0)
                return i;
        } else {
            if (hw_i2c_write(0, msgs[i].addr, msgs[i].buf, msgs[i].len) != 0)
                return i;
        }
    }
    return num;
}

/* Convenience wrappers — real RT-Thread implements these in
 * components/drivers/i2c/i2c_core.c; many generated drivers prefer
 * them over the lower-level rt_i2c_transfer.  */
rt_size_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t addr, rt_uint16_t flags,
                             const rt_uint8_t *buf, rt_uint32_t count) {
    struct rt_i2c_msg msg;
    msg.addr  = addr;
    msg.flags = flags | RT_I2C_WR;
    msg.buf   = (rt_uint8_t *)buf;
    msg.len   = count;
    return rt_i2c_transfer(bus, &msg, 1) == 1 ? count : 0;
}

rt_size_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t addr, rt_uint16_t flags,
                             rt_uint8_t *buf, rt_uint32_t count) {
    struct rt_i2c_msg msg;
    msg.addr  = addr;
    msg.flags = flags | RT_I2C_RD;
    msg.buf   = buf;
    msg.len   = count;
    return rt_i2c_transfer(bus, &msg, 1) == 1 ? count : 0;
}

rt_err_t rt_i2c_control(struct rt_i2c_bus_device *bus,
                        rt_uint32_t cmd,
                        void *arg) {
    (void)bus;
    (void)cmd;
    (void)arg;
    return RT_EOK;
}

/* Official RT-Thread I2C bus lock/unlock: in the real kernel these take
 * &bus->lock via rt_mutex_{take,release}. Our harness is single-threaded
 * so both succeed unconditionally. */
rt_err_t rt_i2c_bus_lock(struct rt_i2c_bus_device *bus, rt_tick_t timeout) {
    (void)bus; (void)timeout;
    return RT_EOK;
}
rt_err_t rt_i2c_bus_unlock(struct rt_i2c_bus_device *bus) {
    (void)bus;
    return RT_EOK;
}

/* Generic device file-ops no-ops. Some drivers call them on fake_bus. */
rt_err_t rt_device_open(rt_device_t dev, rt_uint16_t oflag) { (void)dev; (void)oflag; return RT_EOK; }
rt_err_t rt_device_close(rt_device_t dev) { (void)dev; return RT_EOK; }
rt_err_t rt_device_control(rt_device_t dev, int cmd, void *arg) { (void)dev; (void)cmd; (void)arg; return RT_EOK; }
rt_size_t rt_device_read(rt_device_t dev, rt_off_t pos, void *buf, rt_size_t size) { (void)dev; (void)pos; (void)buf; return size; }
rt_size_t rt_device_write(rt_device_t dev, rt_off_t pos, const void *buf, rt_size_t size) { (void)dev; (void)pos; (void)buf; return size; }

/* ---- Common kernel/printf/pin/hw shims ---- */
#include "hw_uart.h"
#include <stdio.h>
#include <stdarg.h>

rt_err_t rt_thread_mdelay(rt_int32_t ms) {
    if (ms > 0) {
        for (volatile uint32_t i = 0; i < (uint32_t)ms * 800; i++) {}
    }
    return RT_EOK;
}
rt_tick_t rt_tick_get(void) { static rt_tick_t t = 0; return t += 23; }
rt_uint32_t rt_tick_from_millisecond(rt_uint32_t ms) { return ms; }
rt_err_t rt_thread_delay(rt_tick_t tick) { (void)tick; return RT_EOK; }
rt_err_t rt_thread_delay_until(rt_tick_t *tick, rt_tick_t inc_tick) {
    if (tick) *tick += inc_tick;
    return RT_EOK;
}
rt_tick_t rt_tick_get_delta(rt_tick_t since) { (void)since; return 0; }

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
