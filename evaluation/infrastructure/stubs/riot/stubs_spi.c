/* Functional RIOT SPI stubs — spi_transfer_bytes routes through hw_spi.h */
#include "riot.h"
#include "hw_spi.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ── SPI stubs (real STM32 SPI1 via hw_spi.h) ─────────── */

void spi_init(spi_t bus) { (void)bus; hw_spi1_init(); }
void spi_init_cs(spi_t bus, spi_cs_t cs) { (void)bus;(void)cs; }

static bool _spi1_cs_active = false;

static void _spi1_begin_frame(void) {
    if (!_spi1_cs_active) {
        hw_spi1_cs_lo();
        _spi1_cs_active = true;
    }
}

static void _spi1_end_frame_if_needed(bool cont) {
    if (!cont && _spi1_cs_active) {
        hw_spi1_cs_hi();
        _spi1_cs_active = false;
    }
}

int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk) {
    (void)bus;(void)cs;(void)mode;(void)clk; return 0;
}
void spi_release(spi_t bus) {
    (void)bus;
    if (_spi1_cs_active) {
        hw_spi1_cs_hi();
        _spi1_cs_active = false;
    }
}

void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont,
                        const void *out, void *in, size_t len) {
    (void)bus; (void)cs;
    const uint8_t *tx = (const uint8_t *)out;
    uint8_t *rx = (uint8_t *)in;
    _spi1_begin_frame();
    for (size_t i = 0; i < len; i++) {
        uint8_t b = tx ? tx[i] : 0xFF;
        uint8_t r = hw_spi1_xfer_byte(b);
        if (rx) rx[i] = r;
    }
    _spi1_end_frame_if_needed(cont);
}

uint8_t spi_transfer_byte(spi_t bus, spi_cs_t cs, bool cont, uint8_t out) {
    (void)bus; (void)cs;
    _spi1_begin_frame();
    uint8_t r = hw_spi1_xfer_byte(out);
    _spi1_end_frame_if_needed(cont);
    return r;
}

void spi_transfer_regs(spi_t bus, spi_cs_t cs, uint8_t reg,
                       const void *out, void *in, size_t len) {
    (void)bus; (void)cs;
    _spi1_begin_frame();
    hw_spi1_xfer_byte(reg);
    const uint8_t *tx = (const uint8_t *)out;
    uint8_t *rx = (uint8_t *)in;
    for (size_t i = 0; i < len; i++) {
        uint8_t b = tx ? tx[i] : 0xFF;
        uint8_t r = hw_spi1_xfer_byte(b);
        if (rx) rx[i] = r;
    }
    _spi1_end_frame_if_needed(false);
}

uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out) {
    (void)bus; (void)cs;
    _spi1_begin_frame();
    hw_spi1_xfer_byte(reg);
    uint8_t r = hw_spi1_xfer_byte(out);
    _spi1_end_frame_if_needed(false);
    return r;
}

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

/* ── GPIO dummies ─────────────────────────────────────── */
int gpio_init(gpio_t pin, gpio_mode_t mode) { (void)pin;(void)mode; return 0; }
int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank, gpio_cb_t cb, void *arg) { (void)pin;(void)mode;(void)flank;(void)cb;(void)arg; return 0; }
bool gpio_read(gpio_t pin) { (void)pin; return false; }
void gpio_set(gpio_t pin) { (void)pin; }
void gpio_clear(gpio_t pin) { (void)pin; }
void gpio_toggle(gpio_t pin) { (void)pin; }
void gpio_write(gpio_t pin, bool v) { (void)pin;(void)v; }

/* ── UART dummies ─────────────────────────────────────── */
int uart_init(uart_t u, uint32_t b, uart_rx_cb_t cb, void *a) { (void)u;(void)b;(void)cb;(void)a; return 0; }
int uart_mode(uart_t u, uart_data_bits_t d, uart_parity_t p, uart_stop_bits_t s) { (void)u;(void)d;(void)p;(void)s; return 0; }
void uart_write(uart_t u, const uint8_t *d, size_t l) { (void)u;(void)d;(void)l; }
int uart_read(uart_t u, uint8_t *d, size_t l) { (void)u; if(d&&l) memset(d,0,l); return (int)l; }
void uart_poweron(uart_t u) { (void)u; }
void uart_poweroff(uart_t u) { (void)u; }

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
