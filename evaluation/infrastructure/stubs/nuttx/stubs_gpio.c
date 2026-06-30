/* Functional NuttX GPIO stubs — ioctl(GPIOC_WRITE/READ) drives STM32 GPIO MMIO.
 *
 * NuttX uses a character-device model: open("/dev/gpioN") → ioctl(fd, GPIOC_WRITE/READ, &val).
 * We track fd→pin mapping so we can route to the right MMIO bit.
 * Default port: GPIOB (matches Renode pulse-injector wiring).
 */
#include "nuttx.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ── STM32F103 GPIO registers ─────────────────────────── */
#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define GPIOB_BASE_   0x40010C00
#define GPIO_CRL(b)   (*(volatile uint32_t*)((b) + 0x00))
#define GPIO_CRH(b)   (*(volatile uint32_t*)((b) + 0x04))
#define GPIO_IDR(b)   (*(volatile uint32_t*)((b) + 0x08))
#define GPIO_ODR(b)   (*(volatile uint32_t*)((b) + 0x0C))
#define GPIO_BSRR(b)  (*(volatile uint32_t*)((b) + 0x10))
#define IOPBEN  (1U << 3)

/* Track up to 16 open GPIO fds, mapping fd→pin number.
 * open("/dev/gpio5") → pin 5 on GPIOB. */
#define MAX_GPIO_FDS 16
static int _gpio_fd_table[MAX_GPIO_FDS]; /* -1 = unused */
static int _gpio_initialized = 0;

static void _gpio_init_table(void) {
    if (_gpio_initialized) return;
    for (int i = 0; i < MAX_GPIO_FDS; i++) _gpio_fd_table[i] = -1;
    _gpio_initialized = 1;
}

/* Extract pin number from path: "/dev/gpio5" → 5 */
static int _path_to_pin(const char *path) {
    if (!path) return 0;
    /* Find last sequence of digits */
    const char *p = path;
    const char *digits = NULL;
    while (*p) { if (*p >= '0' && *p <= '9') { if (!digits) digits = p; } else digits = NULL; p++; }
    if (digits) {
        int n = 0;
        while (*digits >= '0' && *digits <= '9') { n = n * 10 + (*digits - '0'); digits++; }
        return n;
    }
    return 0;
}

/* ── File I/O stubs (GPIO-functional) ─────────────────── */

int open(const char *path, int oflag, ...) {
    (void)oflag;
    _gpio_init_table();
    RCC_APB2ENR |= IOPBEN;
    int pin = _path_to_pin(path);

    /* Find free fd slot (start from fd 3 to avoid stdin/stdout/stderr) */
    for (int i = 0; i < MAX_GPIO_FDS; i++) {
        if (_gpio_fd_table[i] == -1) {
            _gpio_fd_table[i] = pin;

            /* Configure pin as output (push-pull 10 MHz) by default */
            int shift = (pin < 8 ? pin : pin - 8) * 4;
            volatile uint32_t *reg = (pin < 8)
                ? (volatile uint32_t*)(GPIOB_BASE_ + 0x00)
                : (volatile uint32_t*)(GPIOB_BASE_ + 0x04);
            uint32_t v = *reg;
            v &= ~(0xFU << shift);
            v |= (0x1U << shift);
            *reg = v;

            return i + 3; /* fd offset */
        }
    }
    return -1;
}

int close(int fd) {
    _gpio_init_table();
    int idx = fd - 3;
    if (idx >= 0 && idx < MAX_GPIO_FDS) _gpio_fd_table[idx] = -1;
    return 0;
}

int read(int fd, void *buf, size_t n) {
    (void)fd; if (buf && n) memset(buf, 0, n); return (int)n;
}

int write(int fd, const void *buf, size_t n) {
    (void)fd; (void)buf; return (int)n;
}

/* Heuristic to decide if the ioctl argument was passed by value (e.g. a bool
 * literal 0/1 for GPIOC_WRITE, or an enum gpio_pintype_e small int for
 * GPIOC_SETPINTYPE) vs. by pointer (FAR bool * for GPIOC_READ, or a pointer
 * to a bool that some upstream driver variants use for GPIOC_WRITE).
 *
 * NuttX mainline's canonical convention (per nuttx/ioexpander/gpio.h):
 *   GPIOC_WRITE      - bool value (0/1) passed by value
 *   GPIOC_READ       - FAR bool * (always a pointer)
 *   GPIOC_SETPINTYPE - enum gpio_pintype_e passed by value
 *
 * Our reference driver instead passes &bool for WRITE (a historical variant).
 * Rather than hard-coding the driver's habit into the stub (which would break
 * generated drivers that follow mainline convention), we sniff the argument
 * against the STM32F103 SRAM region (0x20000000..0x20004FFF). Any value with
 * the high bit of 0x20000000 set is treated as a pointer; otherwise as a
 * small integer value. This makes the stub accept BOTH conventions and
 * therefore stays generalisable to any future generated driver. */
#define _ARG_LOOKS_LIKE_PTR(v) (((uintptr_t)(v)) >= 0x20000000u)

int ioctl(int fd, int req, ...) {
    _gpio_init_table();
    int idx = fd - 3;
    int pin = (idx >= 0 && idx < MAX_GPIO_FDS) ? _gpio_fd_table[idx] : 0;

    va_list ap;
    va_start(ap, req);
    unsigned long arg_raw = va_arg(ap, unsigned long);
    va_end(ap);

    if (req == GPIOC_WRITE) {
        bool val;
        if (_ARG_LOOKS_LIKE_PTR(arg_raw)) {
            val = *(bool *)(uintptr_t)arg_raw;
        } else {
            val = (bool)((arg_raw & 0x1u) != 0u);
        }
        if (val)
            GPIO_BSRR(GPIOB_BASE_) = (1U << pin);
        else
            GPIO_BSRR(GPIOB_BASE_) = (1U << (pin + 16));
    } else if (req == GPIOC_READ) {
        if (_ARG_LOOKS_LIKE_PTR(arg_raw)) {
            *(bool *)(uintptr_t)arg_raw = (GPIO_IDR(GPIOB_BASE_) >> pin) & 1U;
        }
    } else if (req == GPIOC_SETPINTYPE) {
        int pintype;
        if (_ARG_LOOKS_LIKE_PTR(arg_raw)) {
            pintype = *(int *)(uintptr_t)arg_raw;
        } else {
            pintype = (int)(arg_raw & 0xFFu);
        }
        int shift = (pin < 8 ? pin : pin - 8) * 4;
        volatile uint32_t *reg = (pin < 8)
            ? (volatile uint32_t*)(GPIOB_BASE_ + 0x00)
            : (volatile uint32_t*)(GPIOB_BASE_ + 0x04);
        uint32_t v = *reg;
        v &= ~(0xFU << shift);
        /* Map NuttX pintype -> STM32F1 CRL/CRH nibble (CNF[3:2] MODE[1:0]).
         * Keeps PP/OD distinction so drivers that rely on open-drain behavior
         * for 1-Wire style buses remain physically correct. */
        uint32_t nibble;
        if (pintype == GPIO_OUTPUT_PIN) {
            nibble = 0x1u;                 /* out PP 10MHz */
        } else if (pintype == GPIO_OUTPUT_PIN_OPENDRAIN) {
            nibble = 0x5u;                 /* out OD 10MHz */
        } else if (pintype == GPIO_INPUT_PIN_PULLUP ||
                   pintype == GPIO_INPUT_PIN_PULLDOWN) {
            nibble = 0x8u;                 /* in with pull-up/down */
        } else if (pintype == GPIO_INPUT_PIN) {
            nibble = 0x4u;                 /* in floating */
        } else {
            nibble = 0x4u;                 /* fallback: floating input */
        }
        v |= (nibble << shift);
        *reg = v;
    }
    return 0;
}

/* ── I2C dummies ──────────────────────────────────────── */
static struct i2c_master_s _i2c_inst;
int I2C_TRANSFER(struct i2c_master_s *dev, struct i2c_msg_s *msgs, int count) { (void)dev;(void)msgs;(void)count; return OK; }
int i2c_write(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR const uint8_t *b, int l) { (void)d;(void)c;(void)b;(void)l; return OK; }
int i2c_read(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR uint8_t *b, int l) { (void)d;(void)c; if(b&&l>0) memset(b,0,l); return OK; }
int i2c_writeread(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR const uint8_t *wb, int wl, FAR uint8_t *rb, int rl) { (void)d;(void)c;(void)wb;(void)wl; if(rb&&rl>0) memset(rb,0,rl); return OK; }
struct i2c_master_s *board_i2cbus_initialize(int bus) { (void)bus; return &_i2c_inst; }
int board_i2cbus_uninitialize(struct i2c_master_s *dev) { (void)dev; return OK; }

/* ── SPI dummies ──────────────────────────────────────── */
void SPI_LOCK(struct spi_dev_s *dev, bool lock) { (void)dev;(void)lock; }
void SPI_SELECT(struct spi_dev_s *dev, uint32_t devid, bool selected) { (void)dev;(void)devid;(void)selected; }
uint32_t SPI_SETFREQUENCY(struct spi_dev_s *dev, uint32_t f) { (void)dev; return f; }
void SPI_SETMODE(struct spi_dev_s *dev, int m) { (void)dev;(void)m; }
void SPI_SETBITS(struct spi_dev_s *dev, int n) { (void)dev;(void)n; }
uint16_t SPI_SEND(struct spi_dev_s *dev, uint16_t wd) { (void)dev;(void)wd; return 0; }
void SPI_EXCHANGE(struct spi_dev_s *dev, const void *tx, void *rx, size_t n) { (void)dev;(void)tx; if(rx&&n) memset(rx,0,n); }
void SPI_SNDBLOCK(struct spi_dev_s *dev, const void *buf, size_t n) { (void)dev;(void)buf;(void)n; }
void SPI_RECVBLOCK(struct spi_dev_s *dev, void *buf, size_t n) { (void)dev; if(buf&&n) memset(buf,0,n); }

/* ── Timing ───────────────────────────────────────────── */
int usleep(unsigned int usec) { (void)usec; return 0; }
unsigned int sleep(unsigned int sec) { (void)sec; return 0; }
int clock_gettime(clockid_t clk_id, struct timespec *tp) { (void)clk_id; if(tp){tp->tv_sec=0;tp->tv_nsec=0;} return 0; }
void up_mdelay(unsigned int ms) { (void)ms; }
void up_udelay(unsigned int us) { (void)us; }

int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}
int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); int n = vsnprintf(buf, size, fmt, ap); va_end(ap); return n;
}
int syslog(int priority, const char *fmt, ...) { (void)priority;(void)fmt; return 0; }

void *kmm_malloc(size_t size) { return malloc(size); }
void *kmm_zalloc(size_t size) { return calloc(1, size); }
void  kmm_free(void *ptr) { free(ptr); }

__attribute__((weak)) int main(void) { return 0; }
