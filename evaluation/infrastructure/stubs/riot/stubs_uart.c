/* Functional RIOT UART stubs — uart_write/uart_read route through hw_uart_bus.h */
#include "riot.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ── UART stubs (real STM32 USART1 via hw_uart_bus.h) ── */

static uart_rx_cb_t _rx_cb = 0;
static void *_rx_cb_arg = 0;

int uart_init(uart_t uart, uint32_t baud, uart_rx_cb_t cb, void *arg) {
    (void)uart; (void)baud;
    _rx_cb = cb;
    _rx_cb_arg = arg;
    hw_uart_bus_init();
    return 0;
}

int uart_mode(uart_t uart, uart_data_bits_t db, uart_parity_t p, uart_stop_bits_t sb) {
    (void)uart; (void)db; (void)p; (void)sb; return 0;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len) {
    (void)uart;
    for (size_t i = 0; i < len; i++)
        hw_uart_bus_write_byte(data[i]);
    /* After TX, poll for response bytes and deliver via callback */
    if (_rx_cb) {
        for (int tries = 0; tries < 64; tries++) {
            uint8_t b;
            if (hw_uart_bus_read_byte(&b) == 0)
                _rx_cb(_rx_cb_arg, b);
            else
                break;
        }
    }
}

int uart_read(uart_t uart, uint8_t *data, size_t len) {
    (void)uart;
    for (size_t i = 0; i < len; i++) {
        if (hw_uart_bus_read_byte(&data[i]) != 0)
            return (int)i;
    }
    return (int)len;
}

void uart_poweron(uart_t uart) { (void)uart; }
void uart_poweroff(uart_t uart) { (void)uart; }

/* ── I2C dummies ──────────────────────────────────────── */
void i2c_init(i2c_t dev) { (void)dev; }
void i2c_acquire(i2c_t dev) { (void)dev; }
void i2c_release(i2c_t dev) { (void)dev; }
int i2c_read_reg(i2c_t d, uint16_t a, uint16_t r, void *data, uint8_t f) { (void)d;(void)a;(void)r;(void)f; if(data) *(uint8_t*)data=0; return 0; }
int i2c_read_regs(i2c_t d, uint16_t a, uint16_t r, void *data, size_t l, uint8_t f) { (void)d;(void)a;(void)r;(void)f; if(data&&l) memset(data,0,l); return 0; }
int i2c_read_byte(i2c_t d, uint16_t a, void *data, uint8_t f) { (void)d;(void)a;(void)f; if(data) *(uint8_t*)data=0; return 0; }
int i2c_read_bytes(i2c_t d, uint16_t a, void *data, size_t l, uint8_t f) { (void)d;(void)a;(void)f; if(data&&l) memset(data,0,l); return 0; }
int i2c_write_byte(i2c_t d, uint16_t a, uint8_t data, uint8_t f) { (void)d;(void)a;(void)data;(void)f; return 0; }
int i2c_write_bytes(i2c_t d, uint16_t a, const void *data, size_t l, uint8_t f) { (void)d;(void)a;(void)data;(void)l;(void)f; return 0; }
int i2c_write_reg(i2c_t d, uint16_t a, uint16_t r, uint8_t data, uint8_t f) { (void)d;(void)a;(void)r;(void)data;(void)f; return 0; }
int i2c_write_regs(i2c_t d, uint16_t a, uint16_t r, const void *data, size_t l, uint8_t f) { (void)d;(void)a;(void)r;(void)data;(void)l;(void)f; return 0; }

/* ── SPI dummies ──────────────────────────────────────── */
void spi_init(spi_t bus) { (void)bus; }
void spi_init_cs(spi_t bus, spi_cs_t cs) { (void)bus;(void)cs; }
int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk) { (void)bus;(void)cs;(void)mode;(void)clk; return 0; }
void spi_release(spi_t bus) { (void)bus; }
void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont, const void *out, void *in, size_t len) { (void)bus;(void)cs;(void)cont;(void)out; if(in&&len) memset(in,0,len); }
uint8_t spi_transfer_byte(spi_t bus, spi_cs_t cs, bool cont, uint8_t out) { (void)bus;(void)cs;(void)cont;(void)out; return 0; }
void spi_transfer_regs(spi_t bus, spi_cs_t cs, uint8_t reg, const void *out, void *in, size_t len) { (void)bus;(void)cs;(void)reg;(void)out; if(in&&len) memset(in,0,len); }
uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out) { (void)bus;(void)cs;(void)reg;(void)out; return 0; }

/* ── GPIO dummies ─────────────────────────────────────── */
int gpio_init(gpio_t pin, gpio_mode_t mode) { (void)pin;(void)mode; return 0; }
int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank, gpio_cb_t cb, void *arg) { (void)pin;(void)mode;(void)flank;(void)cb;(void)arg; return 0; }
bool gpio_read(gpio_t pin) { (void)pin; return false; }
void gpio_set(gpio_t pin) { (void)pin; }
void gpio_clear(gpio_t pin) { (void)pin; }
void gpio_toggle(gpio_t pin) { (void)pin; }
void gpio_write(gpio_t pin, bool v) { (void)pin;(void)v; }

/* ── ztimer / xtimer ──────────────────────────────────── */
static ztimer_clock_t _zu, _zm, _zs;
ztimer_clock_t *ZTIMER_USEC = &_zu, *ZTIMER_MSEC = &_zm, *ZTIMER_SEC = &_zs;
void ztimer_sleep(ztimer_clock_t *c, uint32_t d) { (void)c;(void)d; }
void ztimer_spin(ztimer_clock_t *c, uint32_t d) { (void)c;(void)d; }
uint32_t ztimer_now(ztimer_clock_t *c) { (void)c; return 0; }
void ztimer_set(ztimer_clock_t *c, ztimer_t *t, uint32_t o) { (void)c;(void)t;(void)o; }
void ztimer_periodic_wakeup(ztimer_clock_t *c, uint32_t *l, uint32_t p) { (void)c;(void)l;(void)p; }
void xtimer_usleep(uint32_t us) { (void)us; }
void xtimer_sleep(uint32_t s) { (void)s; }
void xtimer_msleep(uint32_t ms) { (void)ms; }
uint32_t xtimer_now_usec(void) { return 0; }

void mutex_init(mutex_t *m) { (void)m; }
void mutex_lock(mutex_t *m) { (void)m; }
int mutex_trylock(mutex_t *m) { (void)m; return 1; }
void mutex_unlock(mutex_t *m) { (void)m; }
kernel_pid_t thread_create(char *s, int ss, uint8_t p, int f, thread_task_func_t tf, void *a, const char *n) { (void)s;(void)ss;(void)p;(void)f;(void)tf;(void)a;(void)n; return 1; }
void thread_yield(void) {}
kernel_pid_t thread_getpid(void) { return 1; }

int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}
__attribute__((weak)) int main(void) { return 0; }
