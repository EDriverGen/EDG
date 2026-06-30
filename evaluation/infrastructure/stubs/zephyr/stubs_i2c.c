/* Functional Zephyr I2C stubs for the evaluation harness.
 *
 * i2c_write / i2c_read / i2c_write_read route through hw_i2c.h
 * (I2C1 registers) so the driver really drives STM32 I2C1 in Renode.
 * Zephyr uses 7-bit addresses directly.
 */
#include "zephyr.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

bool device_is_ready(const struct device *dev) { (void)dev; return true; }

/* ── I2C stubs (real STM32 I2C1 via hw_i2c.h) ─────────── */

int i2c_write(const struct device *d, const uint8_t *b, uint32_t n, uint16_t a) {
    (void)d;
    return hw_i2c_write(0, (uint8_t)a, b, (uint16_t)n);
}

int i2c_read(const struct device *d, uint8_t *b, uint32_t n, uint16_t a) {
    (void)d;
    return hw_i2c_read(0, (uint8_t)a, b, (uint16_t)n);
}

int i2c_write_read(const struct device *d, uint16_t a,
                   const void *wb, size_t nw, void *rb, size_t nr) {
    (void)d;
    return hw_i2c_write_read(0, (uint8_t)a, wb, (uint16_t)nw, rb, (uint16_t)nr);
}

int i2c_transfer(const struct device *d, struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr) {
    (void)d;
    for (uint8_t i = 0; i < num_msgs; i++) {
        if (msgs[i].flags & I2C_MSG_READ) {
            hw_i2c_read(0, (uint8_t)addr, msgs[i].buf, (uint16_t)msgs[i].len);
        } else {
            hw_i2c_write(0, (uint8_t)addr, msgs[i].buf, (uint16_t)msgs[i].len);
        }
    }
    return 0;
}

int i2c_write_dt(const struct i2c_dt_spec *s, const uint8_t *b, uint32_t n) {
    return i2c_write(s->bus, b, n, s->addr);
}
int i2c_read_dt(const struct i2c_dt_spec *s, uint8_t *b, uint32_t n) {
    return i2c_read(s->bus, b, n, s->addr);
}
int i2c_write_read_dt(const struct i2c_dt_spec *s,
                      const void *wb, size_t nw, void *rb, size_t nr) {
    return i2c_write_read(s->bus, s->addr, wb, nw, rb, nr);
}

int i2c_burst_read(const struct device *d, uint16_t da, uint8_t sa, uint8_t *b, uint32_t n) {
    (void)d;
    return hw_i2c_write_read(0, (uint8_t)da, &sa, 1, b, (uint16_t)n);
}
int i2c_burst_write(const struct device *d, uint16_t da, uint8_t sa, const uint8_t *b, uint32_t n) {
    (void)d;
    uint8_t buf[64];
    buf[0] = sa;
    uint32_t cp = (n < sizeof(buf)-1) ? n : (sizeof(buf)-1);
    if (b && cp) memcpy(buf+1, b, cp);
    return hw_i2c_write(0, (uint8_t)da, buf, (uint16_t)(1+cp));
}
int i2c_reg_read_byte(const struct device *d, uint16_t da, uint8_t ra, uint8_t *v) {
    (void)d;
    return hw_i2c_write_read(0, (uint8_t)da, &ra, 1, v, 1);
}
int i2c_reg_write_byte(const struct device *d, uint16_t da, uint8_t ra, uint8_t v) {
    (void)d;
    uint8_t buf[2] = {ra, v};
    return hw_i2c_write(0, (uint8_t)da, buf, 2);
}
int i2c_reg_read_byte_dt(const struct i2c_dt_spec *s, uint8_t ra, uint8_t *v) {
    return i2c_reg_read_byte(s->bus, s->addr, ra, v);
}
int i2c_reg_write_byte_dt(const struct i2c_dt_spec *s, uint8_t ra, uint8_t v) {
    return i2c_reg_write_byte(s->bus, s->addr, ra, v);
}
int i2c_reg_update_byte_dt(const struct i2c_dt_spec *s, uint8_t ra, uint8_t mask, uint8_t value) {
    uint8_t old = 0;
    int rc = i2c_reg_read_byte(s->bus, s->addr, ra, &old);
    if (rc != 0) return rc;
    uint8_t nv = (old & ~mask) | (value & mask);
    return i2c_reg_write_byte(s->bus, s->addr, ra, nv);
}
int i2c_burst_read_dt(const struct i2c_dt_spec *s, uint8_t sa, uint8_t *b, uint32_t n) {
    return i2c_burst_read(s->bus, s->addr, sa, b, n);
}
int i2c_burst_write_dt(const struct i2c_dt_spec *s, uint8_t sa, const uint8_t *b, uint32_t n) {
    return i2c_burst_write(s->bus, s->addr, sa, b, n);
}
int i2c_transfer_dt(const struct i2c_dt_spec *s, struct i2c_msg *msgs, uint8_t num_msgs) {
    return i2c_transfer(s->bus, msgs, num_msgs, s->addr);
}

/* ── GPIO dummies ─────────────────────────────────────── */
int gpio_pin_configure(const struct device *p, gpio_pin_t pin, gpio_flags_t f) { (void)p;(void)pin;(void)f; return 0; }
int gpio_pin_configure_dt(const struct gpio_dt_spec *s, gpio_flags_t f) { (void)s;(void)f; return 0; }
int gpio_pin_set(const struct device *p, gpio_pin_t pin, int v) { (void)p;(void)pin;(void)v; return 0; }
int gpio_pin_set_dt(const struct gpio_dt_spec *s, int v) { (void)s;(void)v; return 0; }
int gpio_pin_get(const struct device *p, gpio_pin_t pin) { (void)p;(void)pin; return 0; }
int gpio_pin_get_dt(const struct gpio_dt_spec *s) { (void)s; return 0; }
int gpio_pin_toggle_dt(const struct gpio_dt_spec *s) { (void)s; return 0; }

/* ── SPI dummies ──────────────────────────────────────── */
int spi_transceive(const struct device *d, const struct spi_config *c, const struct spi_buf_set *tx, const struct spi_buf_set *rx) { (void)d;(void)c;(void)tx; if(rx&&rx->count>0&&rx->buffers[0].buf) memset(rx->buffers[0].buf,0,rx->buffers[0].len); return 0; }
int spi_transceive_dt(const struct spi_dt_spec *s, const struct spi_buf_set *tx, const struct spi_buf_set *rx) { return spi_transceive(s->bus, &s->config, tx, rx); }
int spi_read(const struct device *d, const struct spi_config *c, const struct spi_buf_set *rx) { return spi_transceive(d, c, 0, rx); }
int spi_read_dt(const struct spi_dt_spec *s, const struct spi_buf_set *rx) { return spi_transceive(s->bus, &s->config, 0, rx); }
int spi_write(const struct device *d, const struct spi_config *c, const struct spi_buf_set *tx) { return spi_transceive(d, c, tx, 0); }
int spi_write_dt(const struct spi_dt_spec *s, const struct spi_buf_set *tx) { return spi_transceive(s->bus, &s->config, tx, 0); }

/* ── UART dummies ─────────────────────────────────────── */
int uart_configure(const struct device *d, const struct uart_config *c) { (void)d;(void)c; return 0; }
int uart_config_get(const struct device *d, struct uart_config *c) { (void)d;(void)c; return 0; }
void uart_poll_out(const struct device *d, unsigned char c) { (void)d;(void)c; }
int uart_poll_in(const struct device *d, unsigned char *c) { (void)d;(void)c; return -1; }
int uart_fifo_fill(const struct device *d, const uint8_t *t, int s) { (void)d;(void)t; return s; }
int uart_fifo_read(const struct device *d, uint8_t *r, int s) { (void)d; if(r&&s>0) memset(r,0,s); return s; }
void uart_irq_rx_enable(const struct device *d) { (void)d; }
void uart_irq_rx_disable(const struct device *d) { (void)d; }
void uart_irq_tx_enable(const struct device *d) { (void)d; }
void uart_irq_tx_disable(const struct device *d) { (void)d; }
int uart_irq_rx_ready(const struct device *d) { (void)d; return 0; }
int uart_irq_tx_ready(const struct device *d) { (void)d; return 1; }

/* ── Kernel timing ────────────────────────────────────── */
void k_msleep(int32_t ms) { (void)ms; }
void k_usleep(int32_t us) { (void)us; }
void k_busy_wait(uint32_t usec) { (void)usec; }
int64_t k_uptime_get(void) { return 0; }
uint32_t k_uptime_get_32(void) { return 0; }
k_ticks_t k_uptime_ticks(void) { return 0; }
int32_t k_sleep(k_timeout_t timeout) { (void)timeout; return 0; }

/* ── printf via UART2 ─────────────────────────────────── */
int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}

__attribute__((weak)) int main(void) { return 0; }

/* device_get_binding(name): device lookup. Returns non-NULL sentinel. */
const struct device *device_get_binding(const char *name) {
    static const struct device _dummy = { .name = "stub", .config = 0, .api = 0, .data = 0 };
    (void)name;
    return &_dummy;
}
