/* Functional RIOT I2C stubs — i2c_read/i2c_write route through hw_i2c.h */
#include "riot.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ── I2C stubs (real STM32 I2C1 via hw_i2c.h) ─────────── */

void i2c_init(i2c_t dev) { (void)dev; hw_i2c1_init(); }
void i2c_acquire(i2c_t dev) { (void)dev; }
void i2c_release(i2c_t dev) { (void)dev; }

int i2c_read_reg(i2c_t dev, uint16_t addr, uint16_t reg, void *data, uint8_t flags) {
    (void)dev; (void)flags;
    uint8_t r = (uint8_t)(reg & 0xFF);
    return hw_i2c_write_read(0, (uint8_t)addr, &r, 1, data, 1);
}

int i2c_read_regs(i2c_t dev, uint16_t addr, uint16_t reg, void *data, size_t len, uint8_t flags) {
    (void)dev; (void)flags;
    uint8_t r = (uint8_t)(reg & 0xFF);
    return hw_i2c_write_read(0, (uint8_t)addr, &r, 1, data, (uint16_t)len);
}

int i2c_read_byte(i2c_t dev, uint16_t addr, void *data, uint8_t flags) {
    (void)dev; (void)flags;
    return hw_i2c_read(0, (uint8_t)addr, data, 1);
}

int i2c_read_bytes(i2c_t dev, uint16_t addr, void *data, size_t len, uint8_t flags) {
    (void)dev; (void)flags;
    return hw_i2c_read(0, (uint8_t)addr, data, (uint16_t)len);
}

int i2c_write_byte(i2c_t dev, uint16_t addr, uint8_t data, uint8_t flags) {
    (void)dev; (void)flags;
    return hw_i2c_write(0, (uint8_t)addr, &data, 1);
}

int i2c_write_bytes(i2c_t dev, uint16_t addr, const void *data, size_t len, uint8_t flags) {
    (void)dev; (void)flags;
    return hw_i2c_write(0, (uint8_t)addr, data, (uint16_t)len);
}

int i2c_write_reg(i2c_t dev, uint16_t addr, uint16_t reg, uint8_t data, uint8_t flags) {
    (void)dev; (void)flags;
    uint8_t buf[2] = {(uint8_t)(reg & 0xFF), data};
    return hw_i2c_write(0, (uint8_t)addr, buf, 2);
}

int i2c_write_regs(i2c_t dev, uint16_t addr, uint16_t reg, const void *data, size_t len, uint8_t flags) {
    (void)dev; (void)flags;
    uint8_t buf[64];
    buf[0] = (uint8_t)(reg & 0xFF);
    size_t cp = (len < sizeof(buf)-1) ? len : (sizeof(buf)-1);
    if (data && cp) memcpy(buf+1, data, cp);
    return hw_i2c_write(0, (uint8_t)addr, buf, (uint16_t)(1+cp));
}

/* ── GPIO dummies ─────────────────────────────────────── */
int gpio_init(gpio_t pin, gpio_mode_t mode) { (void)pin;(void)mode; return 0; }
int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank, gpio_cb_t cb, void *arg) { (void)pin;(void)mode;(void)flank;(void)cb;(void)arg; return 0; }
bool gpio_read(gpio_t pin) { (void)pin; return false; }
void gpio_set(gpio_t pin) { (void)pin; }
void gpio_clear(gpio_t pin) { (void)pin; }
void gpio_toggle(gpio_t pin) { (void)pin; }
void gpio_write(gpio_t pin, bool value) { (void)pin;(void)value; }

/* ── SPI dummies ──────────────────────────────────────── */
void spi_init(spi_t bus) { (void)bus; }
void spi_init_cs(spi_t bus, spi_cs_t cs) { (void)bus;(void)cs; }
int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk) { (void)bus;(void)cs;(void)mode;(void)clk; return 0; }
void spi_release(spi_t bus) { (void)bus; }
void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont, const void *out, void *in, size_t len) { (void)bus;(void)cs;(void)cont;(void)out; if(in&&len) memset(in,0,len); }
uint8_t spi_transfer_byte(spi_t bus, spi_cs_t cs, bool cont, uint8_t out) { (void)bus;(void)cs;(void)cont;(void)out; return 0; }
void spi_transfer_regs(spi_t bus, spi_cs_t cs, uint8_t reg, const void *out, void *in, size_t len) { (void)bus;(void)cs;(void)reg;(void)out; if(in&&len) memset(in,0,len); }
uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out) { (void)bus;(void)cs;(void)reg;(void)out; return 0; }

/* ── UART dummies ─────────────────────────────────────── */
int uart_init(uart_t uart, uint32_t baud, uart_rx_cb_t cb, void *arg) { (void)uart;(void)baud;(void)cb;(void)arg; return 0; }
int uart_mode(uart_t uart, uart_data_bits_t db, uart_parity_t p, uart_stop_bits_t sb) { (void)uart;(void)db;(void)p;(void)sb; return 0; }
void uart_write(uart_t uart, const uint8_t *data, size_t len) { (void)uart;(void)data;(void)len; }
int uart_read(uart_t uart, uint8_t *data, size_t len) { (void)uart; if(data&&len) memset(data,0,len); return (int)len; }
void uart_poweron(uart_t uart) { (void)uart; }
void uart_poweroff(uart_t uart) { (void)uart; }

/* ── ztimer / xtimer ──────────────────────────────────── */
static ztimer_clock_t _ztimer_usec_inst, _ztimer_msec_inst, _ztimer_sec_inst;
ztimer_clock_t *ZTIMER_USEC = &_ztimer_usec_inst;
ztimer_clock_t *ZTIMER_MSEC = &_ztimer_msec_inst;
ztimer_clock_t *ZTIMER_SEC  = &_ztimer_sec_inst;
void ztimer_sleep(ztimer_clock_t *c, uint32_t d) { (void)c;(void)d; }
void ztimer_spin(ztimer_clock_t *c, uint32_t d) { (void)c;(void)d; }
uint32_t ztimer_now(ztimer_clock_t *c) { (void)c; return 0; }
void ztimer_set(ztimer_clock_t *c, ztimer_t *t, uint32_t o) { (void)c;(void)t;(void)o; }
void ztimer_periodic_wakeup(ztimer_clock_t *c, uint32_t *l, uint32_t p) { (void)c;(void)l;(void)p; }
void xtimer_usleep(uint32_t us) { (void)us; }
void xtimer_sleep(uint32_t s) { (void)s; }
void xtimer_msleep(uint32_t ms) { (void)ms; }
uint32_t xtimer_now_usec(void) { return 0; }

/* ── Mutex / Thread ───────────────────────────────────── */
void mutex_init(mutex_t *m) { (void)m; }
void mutex_lock(mutex_t *m) { (void)m; }
int mutex_trylock(mutex_t *m) { (void)m; return 1; }
void mutex_unlock(mutex_t *m) { (void)m; }
kernel_pid_t thread_create(char *s, int ss, uint8_t p, int f, thread_task_func_t tf, void *a, const char *n) { (void)s;(void)ss;(void)p;(void)f;(void)tf;(void)a;(void)n; return 1; }
void thread_yield(void) {}
kernel_pid_t thread_getpid(void) { return 1; }

/* ── printf via UART2 ─────────────────────────────────── */
int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
