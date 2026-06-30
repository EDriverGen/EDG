/* Functional Zephyr UART stubs — uart_poll_out/in route through hw_uart_bus.h */
#include "zephyr.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

bool device_is_ready(const struct device *dev) { (void)dev; return true; }

/* ── UART stubs (real STM32 USART1 via hw_uart_bus.h) ─── */

int uart_configure(const struct device *dev, const struct uart_config *cfg) {
    (void)dev; (void)cfg; hw_uart_bus_init(); return 0;
}
int uart_config_get(const struct device *dev, struct uart_config *cfg) { (void)dev;(void)cfg; return 0; }

void uart_poll_out(const struct device *dev, unsigned char out_char) {
    (void)dev;
    hw_uart_bus_write_byte(out_char);
}

int uart_poll_in(const struct device *dev, unsigned char *p_char) {
    (void)dev;
    return hw_uart_bus_read_byte((uint8_t*)p_char);
}

int uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size) {
    (void)dev;
    for (int i = 0; i < size; i++) hw_uart_bus_write_byte(tx_data[i]);
    return size;
}

int uart_fifo_read(const struct device *dev, uint8_t *rx_data, int size) {
    (void)dev;
    int got = 0;
    for (int i = 0; i < size; i++) {
        if (hw_uart_bus_read_byte(&rx_data[i]) != 0) break;
        got++;
    }
    return got;
}

void uart_irq_rx_enable(const struct device *d) { (void)d; }
void uart_irq_rx_disable(const struct device *d) { (void)d; }
void uart_irq_tx_enable(const struct device *d) { (void)d; }
void uart_irq_tx_disable(const struct device *d) { (void)d; }
int uart_irq_rx_ready(const struct device *d) { (void)d; return 0; }
int uart_irq_tx_ready(const struct device *d) { (void)d; return 1; }

/* ── I2C dummies ──────────────────────────────────────── */
int i2c_write(const struct device *d, const uint8_t *b, uint32_t n, uint16_t a) { (void)d;(void)b;(void)n;(void)a; return 0; }
int i2c_read(const struct device *d, uint8_t *b, uint32_t n, uint16_t a) { (void)d;(void)a; if(b&&n) memset(b,0,n); return 0; }
int i2c_write_read(const struct device *d, uint16_t a, const void *wb, size_t nw, void *rb, size_t nr) { (void)d;(void)a;(void)wb;(void)nw; if(rb&&nr) memset(rb,0,nr); return 0; }
int i2c_transfer(const struct device *d, struct i2c_msg *m, uint8_t n, uint16_t a) { (void)d;(void)m;(void)n;(void)a; return 0; }
int i2c_write_dt(const struct i2c_dt_spec *s, const uint8_t *b, uint32_t n) { (void)s;(void)b;(void)n; return 0; }
int i2c_read_dt(const struct i2c_dt_spec *s, uint8_t *b, uint32_t n) { (void)s; if(b&&n) memset(b,0,n); return 0; }
int i2c_write_read_dt(const struct i2c_dt_spec *s, const void *wb, size_t nw, void *rb, size_t nr) { (void)s;(void)wb;(void)nw; if(rb&&nr) memset(rb,0,nr); return 0; }
int i2c_burst_read(const struct device *d, uint16_t da, uint8_t sa, uint8_t *b, uint32_t n) { (void)d;(void)da;(void)sa; if(b&&n) memset(b,0,n); return 0; }
int i2c_burst_write(const struct device *d, uint16_t da, uint8_t sa, const uint8_t *b, uint32_t n) { (void)d;(void)da;(void)sa;(void)b;(void)n; return 0; }
int i2c_reg_read_byte(const struct device *d, uint16_t da, uint8_t ra, uint8_t *v) { (void)d;(void)da;(void)ra; if(v) *v=0; return 0; }
int i2c_reg_write_byte(const struct device *d, uint16_t da, uint8_t ra, uint8_t v) { (void)d;(void)da;(void)ra;(void)v; return 0; }
int i2c_reg_read_byte_dt(const struct i2c_dt_spec *s, uint8_t ra, uint8_t *v) { (void)s;(void)ra; if(v) *v=0; return 0; }
int i2c_reg_write_byte_dt(const struct i2c_dt_spec *s, uint8_t ra, uint8_t v) { (void)s;(void)ra;(void)v; return 0; }

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
int spi_read(const struct device *d, const struct spi_config *c, const struct spi_buf_set *rx) { return spi_transceive(d,c,0,rx); }
int spi_read_dt(const struct spi_dt_spec *s, const struct spi_buf_set *rx) { return spi_transceive(s->bus,&s->config,0,rx); }
int spi_write(const struct device *d, const struct spi_config *c, const struct spi_buf_set *tx) { return spi_transceive(d,c,tx,0); }
int spi_write_dt(const struct spi_dt_spec *s, const struct spi_buf_set *tx) { return spi_transceive(s->bus,&s->config,tx,0); }

/* ── Kernel timing ────────────────────────────────────── */
void k_msleep(int32_t ms) { (void)ms; }
void k_usleep(int32_t us) { (void)us; }
void k_busy_wait(uint32_t usec) { (void)usec; }
int64_t k_uptime_get(void) { return 0; }
uint32_t k_uptime_get_32(void) { return 0; }
k_ticks_t k_uptime_ticks(void) { return 0; }

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

int32_t k_sleep(k_timeout_t timeout) { (void)timeout; return 0; }
