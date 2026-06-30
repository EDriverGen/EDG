/* Functional RIOT GPIO stubs — gpio_set/clear/read/toggle route to STM32 GPIO MMIO.
 *
 * RIOT encodes GPIO pins as: GPIO_PIN(port, pin) = (port << 8 | pin)
 * Extract: port = gpio_t >> 8, pin_num = gpio_t & 0xFF
 * Map: port 0→GPIOA, 1→GPIOB, 2→GPIOC, 3→GPIOD, 4→GPIOE
 */
#include "riot.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ── STM32F103 GPIO registers ─────────────────────────── */
#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define GPIOA_BASE_   0x40010800
#define GPIOB_BASE_   0x40010C00
#define GPIOC_BASE_   0x40011000
#define GPIOD_BASE_   0x40011400
#define GPIOE_BASE_   0x40011800

#define GPIO_CRL(b)   (*(volatile uint32_t*)((b) + 0x00))
#define GPIO_CRH(b)   (*(volatile uint32_t*)((b) + 0x04))
#define GPIO_IDR(b)   (*(volatile uint32_t*)((b) + 0x08))
#define GPIO_ODR(b)   (*(volatile uint32_t*)((b) + 0x0C))
#define GPIO_BSRR(b)  (*(volatile uint32_t*)((b) + 0x10))

static const uint32_t _port_base[5] = {
    GPIOA_BASE_, GPIOB_BASE_, GPIOC_BASE_, GPIOD_BASE_, GPIOE_BASE_
};

static inline uint32_t _get_base(gpio_t pin) {
    unsigned port = (unsigned)pin >> 8;
    return (port < 5) ? _port_base[port] : GPIOB_BASE_;
}

static inline uint32_t _get_pin(gpio_t pin) {
    return (unsigned)pin & 0xFF;
}

/* ── GPIO stubs (real MMIO) ───────────────────────────── */

int gpio_init(gpio_t pin, gpio_mode_t mode) {
    unsigned port = (unsigned)pin >> 8;
    unsigned pn = (unsigned)pin & 0xFF;
    if (port > 4 || pn > 15) return -1;

    /* Enable clock: IOPAEN=bit2, IOPBEN=bit3, ..., IOPEEN=bit6 */
    RCC_APB2ENR |= (1U << (port + 2));

    uint32_t base = _port_base[port];
    int shift = (pn < 8 ? pn : pn - 8) * 4;
    volatile uint32_t *reg = (pn < 8)
        ? (volatile uint32_t*)(base + 0x00)
        : (volatile uint32_t*)(base + 0x04);

    uint32_t v = *reg;
    v &= ~(0xFU << shift);

    /* Map RIOT gpio_mode_t to STM32F103 CRL/CRH MODE/CNF nibbles. */
    uint32_t nibble;
    switch (mode) {
        case GPIO_OUT:                       nibble = 0x1u; break; /* out PP 10MHz */
        case GPIO_OD:
        case GPIO_OD_PU:                     nibble = 0x5u; break; /* out OD 10MHz */
        case GPIO_IN:                        nibble = 0x4u; break; /* in floating */
        case GPIO_IN_PU:
        case GPIO_IN_PD:                     nibble = 0x8u; break; /* in pulled */
        default:                             nibble = 0x4u; break;
    }
    v |= (nibble << shift);

    *reg = v;
    return 0;
}

int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank,
                  gpio_cb_t cb, void *arg) {
    (void)flank; (void)cb; (void)arg;
    return gpio_init(pin, mode);
}

bool gpio_read(gpio_t pin) {
    return (GPIO_IDR(_get_base(pin)) >> _get_pin(pin)) & 1U;
}

void gpio_set(gpio_t pin) {
    GPIO_BSRR(_get_base(pin)) = 1U << _get_pin(pin);
}

void gpio_clear(gpio_t pin) {
    GPIO_BSRR(_get_base(pin)) = 1U << (_get_pin(pin) + 16);
}

void gpio_toggle(gpio_t pin) {
    uint32_t base = _get_base(pin);
    uint32_t pn = _get_pin(pin);
    GPIO_ODR(base) ^= (1U << pn);
}

void gpio_write(gpio_t pin, bool value) {
    if (value) gpio_set(pin);
    else gpio_clear(pin);
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
