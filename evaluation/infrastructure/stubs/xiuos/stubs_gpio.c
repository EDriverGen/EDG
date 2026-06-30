/* Functional XiUOS GPIO stubs — PrivIoctl with PinParam / GpioConfigParam route to STM32 GPIO MMIO.
 *
 * XiUOS drivers use PrivOpen("/dev/pin") + PrivIoctl(fd, cmd, &PinParam) for GPIO.
 * We track pin state and route to real GPIOB registers.
 */
#include "xiuos.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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
#define IOPBEN  (1U << 3)

/* Current pin tracked by the most recent PrivIoctl configure call */
static uint16_t _cur_pin = 0;

/* fd→pin mapping for multi-pin drivers (e.g. hcsr04 trig+echo) */
#define MAX_PRIV_FDS 16
static int _fd_pin[MAX_PRIV_FDS]; /* index = fd - 3, value = pin */
static int _fd_used[MAX_PRIV_FDS];
static int _fd_init_done = 0;
static void _fd_init(void) {
    if (_fd_init_done) return;
    for (int i = 0; i < MAX_PRIV_FDS; i++) { _fd_pin[i] = -1; _fd_used[i] = 0; }
    _fd_init_done = 1;
}

/* ── Transform layer stubs (with GPIO routing) ────────── */

int PrivOpen(const char *path, int oflag) {
    (void)oflag;
    _fd_init();
    RCC_APB2ENR |= IOPBEN; /* enable GPIOB clock */
    int pin = 0;
    /* Parse pin from path, e.g. "PB5" → pin 5, "/dev/pin/PB5" → pin 5 */
    if (path) {
        const char *last_p = 0;
        for (const char *s = path; *s; s++)
            if (*s == 'P') last_p = s;
        if (last_p && last_p[1] >= 'A' && last_p[1] <= 'H') {
            pin = 0;
            for (int i = 2; last_p[i] >= '0' && last_p[i] <= '9'; i++)
                pin = pin * 10 + (last_p[i] - '0');
            if (pin > 15) pin = 0;
        }
    }
    _cur_pin = (uint16_t)pin;
    /* Find free fd slot */
    for (int i = 0; i < MAX_PRIV_FDS; i++) {
        if (!_fd_used[i]) {
            _fd_used[i] = 1;
            _fd_pin[i] = pin;
            return i + 3;
        }
    }
    return 3;
}

int PrivClose(int fd) {
    _fd_init();
    int idx = fd - 3;
    if (idx >= 0 && idx < MAX_PRIV_FDS) { _fd_used[idx] = 0; _fd_pin[idx] = -1; }
    return 0;
}

static uint16_t _pin_for_fd(int fd) {
    int idx = fd - 3;
    if (idx >= 0 && idx < MAX_PRIV_FDS && _fd_used[idx]) return (uint16_t)_fd_pin[idx];
    return _cur_pin;
}

int PrivRead(int fd, void *buf, size_t n) {
    (void)n;
    if (buf) {
        uint16_t pin = _pin_for_fd(fd);
        uint32_t val = (GPIO_IDR(GPIOB_BASE_) >> pin) & 1U;
        *(uint8_t *)buf = (uint8_t)val;
    }
    return 1;
}

int PrivWrite(int fd, const void *buf, size_t n) {
    (void)n;
    if (buf) {
        uint16_t pin = _pin_for_fd(fd);
        uint8_t val = *(const uint8_t *)buf;
        if (val)
            GPIO_BSRR(GPIOB_BASE_) = (1U << pin);
        else
            GPIO_BSRR(GPIOB_BASE_) = (1U << (pin + 16));
    }
    return 1;
}

int PrivIoctl(int fd, int cmd, void *arg) {
    _fd_init();
    /* XiUOS convention: cmd may be 0 with actual command inside arg */
    int effective_cmd = cmd;
    if (cmd == 0 && arg) {
        struct PinParam *pp = (struct PinParam *)arg;
        effective_cmd = pp->cmd;
    }

    if (effective_cmd == GPIO_CONFIG_MODE && arg) {
        /* Configure pin mode.
         *
         * Pin source precedence — avoid trusting driver-supplied PinParam.pin
         * unconditionally: the XiUOS idiom is to open "/dev/pin/PBx" so the
         * fd already encodes the pin, and well-behaved drivers leave
         * PinParam.pin uninitialized. Reading an uninitialized stack slot
         * can hand us a bogus pin in [1..15] that then clobbers the wrong
         * port register (observed on ds18b20 ref: last READ_SCRATCH bit's
         * set_input() silently retargeted another pin, leaving PB5 stuck in
         * OUTPUT mode and all subsequent IDR polls returning 0).
         *
         * Rule:
         *   1. If fd has an active mapping from PrivOpen, trust it.
         *   2. Otherwise, fall back to PinParam.pin when it is a legal value.
         *   3. Otherwise, use the last tracked _cur_pin.
         *
         * This is generic: any driver that properly calls PrivOpen before
         * PrivIoctl (as every XiUOS driver must) gets the correct pin; only
         * drivers that bypass PrivOpen have to set PinParam.pin explicitly,
         * which is a reasonable contract for that edge case.
         */
        struct PinParam *pp = (struct PinParam *)arg;
        int idx = fd - 3;
        uint16_t pin;
        if (idx >= 0 && idx < MAX_PRIV_FDS && _fd_used[idx]) {
            pin = (uint16_t)_fd_pin[idx];
        } else if (pp->pin <= 15) {
            pin = pp->pin;
        } else {
            pin = _cur_pin;
        }
        _cur_pin = pin;

        int shift = (pin < 8 ? pin : pin - 8) * 4;
        volatile uint32_t *reg = (pin < 8)
            ? (volatile uint32_t*)(GPIOB_BASE_ + 0x00)
            : (volatile uint32_t*)(GPIOB_BASE_ + 0x04);

        uint32_t v = *reg;
        v &= ~(0xFU << shift);
        /* Map XiUOS GPIO_CFG_* to STM32F1 CRL/CRH nibble (CNF[3:2] MODE[1:0]).
         * Preserve push-pull vs open-drain so 1-Wire style buses (DS18B20,
         * DHT22 when emulated via GPIO) behave physically correctly even
         * when the simulated pin is driven by BSRR writes from the driver. */
        uint32_t nibble;
        if (pp->mode == GPIO_CFG_OUTPUT)               nibble = 0x1u; /* out PP 10MHz */
        else if (pp->mode == GPIO_CFG_OUTPUT_OD)       nibble = 0x5u; /* out OD 10MHz */
        else if (pp->mode == GPIO_CFG_INPUT_PULLUP ||
                 pp->mode == GPIO_CFG_INPUT_PULLDOWN)  nibble = 0x8u; /* in pulled */
        else if (pp->mode == GPIO_CFG_INPUT)           nibble = 0x4u; /* in floating */
        else                                           nibble = 0x4u; /* safe default */
        v |= (nibble << shift);
        *reg = v;
        return 0;
    }

    if (arg) {
        /* Some drivers pass GpioConfigParam or gpio_param for set/clear */
        GpioConfigParam *gp = (GpioConfigParam *)arg;
        if (gp->cmd == GPIO_CONFIG_MODE) {
            _cur_pin = gp->pin;
        }
    }
    return 0;
}

void PrivTaskDelay(int32_t ms) { (void)ms; }

/* ── Mutex / Semaphore ────────────────────────────────── */
int PrivMutexCreate(void **m, int a) { (void)m;(void)a; return 0; }
int PrivMutexDelete(void *m) { (void)m; return 0; }
int PrivMutexObtain(void *m) { (void)m; return 0; }
int PrivMutexAbandon(void *m) { (void)m; return 0; }
int PrivSemaphoreCreate(void **s, int a, int c) { (void)s;(void)a;(void)c; return 0; }
int PrivSemaphoreDelete(void *s) { (void)s; return 0; }
int PrivSemaphoreObtainWait(void *s, int32_t ms) { (void)s;(void)ms; return 0; }
int PrivSemaphoreAbandon(void *s) { (void)s; return 0; }

/* ── Bus framework ────────────────────────────────────── */
BusType BusFind(const char *name) { (void)name; return (BusType)1; }
HardwareDevType BusFindDevice(BusType bus, const char *name) { (void)bus;(void)name; return (HardwareDevType)1; }
int BusDevOpen(HardwareDevType dev) { (void)dev; return 0; }
int BusDevClose(HardwareDevType dev) { (void)dev; return 0; }
int BusDevWriteData(HardwareDevType dev, struct BusBlockWriteParam *wp) { (void)dev;(void)wp; return 0; }
int BusDevReadData(HardwareDevType dev, struct BusBlockReadParam *rp) { (void)dev;(void)rp; return 0; }
int BusDrvConfigure(HardwareDevType dev, void *cfg) { (void)dev;(void)cfg; return 0; }

/* ── Memory ───────────────────────────────────────────── */
void *PrivMalloc(size_t sz) { return malloc(sz); }
void *PrivCalloc(size_t n, size_t sz) { return calloc(n, sz); }
void PrivFree(void *p) { free(p); }

/* ── Device-specific delay_us (GPIO sensor drivers) ──── */
void dht22_delay_us(uint32_t us) { (void)us; }
void ds18b20_delay_us(uint32_t us) { (void)us; }
void hcsr04_delay_us(uint32_t us) { (void)us; }
uint32_t hcsr04_get_us_tick(void) { static uint32_t t = 0; return t += 1; }

/* ── printf via UART2 ─────────────────────────────────── */
int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
