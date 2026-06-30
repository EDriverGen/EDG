/* Functional Zephyr GPIO stubs — gpio_pin_set/get drive STM32 GPIO registers.
 *
 * Since Zephyr's device model uses opaque `const struct device *` for port,
 * and the stub's _fake_device doesn't carry port info, we default GPIO ops
 * to GPIOB (which is what the Renode pulse-injector wires to).
 */
#include "zephyr.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

bool device_is_ready(const struct device *dev) { (void)dev; return true; }

/* ── STM32F103 GPIO registers ─────────────────────────── */
#define RCC_APB2ENR  (*(volatile uint32_t*)0x40021018)
#define GPIOB_BASE_  0x40010C00
#define GPIO_CRL(b)  (*(volatile uint32_t*)((b) + 0x00))
#define GPIO_CRH(b)  (*(volatile uint32_t*)((b) + 0x04))
#define GPIO_IDR(b)  (*(volatile uint32_t*)((b) + 0x08))
#define GPIO_ODR(b)  (*(volatile uint32_t*)((b) + 0x0C))
#define GPIO_BSRR(b) (*(volatile uint32_t*)((b) + 0x10))
#define IOPBEN (1U << 3)

/* ── GPIO stubs (real MMIO, defaulting to GPIOB) ──────── */

int gpio_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags) {
    (void)port;
    RCC_APB2ENR |= IOPBEN;
    int shift = (pin < 8 ? pin : pin - 8) * 4;
    volatile uint32_t *reg = (pin < 8)
        ? (volatile uint32_t*)(GPIOB_BASE_ + 0x00)
        : (volatile uint32_t*)(GPIOB_BASE_ + 0x04);
    uint32_t v = *reg;
    v &= ~(0xFU << shift);
    if (flags & GPIO_OUTPUT) {
        v |= (0x1U << shift);
        *reg = v;
        /* Drive initial level via BSRR */
        if (flags & (1 << 5))  /* GPIO_OUTPUT_INIT_HIGH */
            GPIO_BSRR(GPIOB_BASE_) = (1U << pin);
        else
            GPIO_BSRR(GPIOB_BASE_) = (1U << (pin + 16));  /* default LOW */
    } else {
        v |= (0x4U << shift);
        *reg = v;
    }
    return 0;
}

int gpio_pin_configure_dt(const struct gpio_dt_spec *spec, gpio_flags_t extra_flags) {
    return gpio_pin_configure(spec->port, spec->pin, spec->dt_flags | extra_flags);
}

int gpio_pin_set(const struct device *port, gpio_pin_t pin, int value) {
    (void)port;
    if (value) GPIO_BSRR(GPIOB_BASE_) = (1U << pin);
    else       GPIO_BSRR(GPIOB_BASE_) = (1U << (pin + 16));
    return 0;
}

int gpio_pin_set_dt(const struct gpio_dt_spec *spec, int value) {
    return gpio_pin_set(spec->port, spec->pin, value);
}

int gpio_pin_get(const struct device *port, gpio_pin_t pin) {
    (void)port;
    return (GPIO_IDR(GPIOB_BASE_) >> pin) & 1U;
}

int gpio_pin_get_dt(const struct gpio_dt_spec *spec) {
    return gpio_pin_get(spec->port, spec->pin);
}

int gpio_pin_toggle_dt(const struct gpio_dt_spec *spec) {
    uint32_t pin = spec->pin;
    GPIO_ODR(GPIOB_BASE_) ^= (1U << pin);
    return 0;
}

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

/* ── SPI dummies ──────────────────────────────────────── */
int spi_transceive(const struct device *d, const struct spi_config *c, const struct spi_buf_set *tx, const struct spi_buf_set *rx) { (void)d;(void)c;(void)tx; if(rx&&rx->count>0&&rx->buffers[0].buf) memset(rx->buffers[0].buf,0,rx->buffers[0].len); return 0; }
int spi_transceive_dt(const struct spi_dt_spec *s, const struct spi_buf_set *tx, const struct spi_buf_set *rx) { return spi_transceive(s->bus,&s->config,tx,rx); }
int spi_read(const struct device *d, const struct spi_config *c, const struct spi_buf_set *rx) { return spi_transceive(d,c,0,rx); }
int spi_read_dt(const struct spi_dt_spec *s, const struct spi_buf_set *rx) { return spi_transceive(s->bus,&s->config,0,rx); }
int spi_write(const struct device *d, const struct spi_config *c, const struct spi_buf_set *tx) { return spi_transceive(d,c,tx,0); }
int spi_write_dt(const struct spi_dt_spec *s, const struct spi_buf_set *tx) { return spi_transceive(s->bus,&s->config,tx,0); }

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
