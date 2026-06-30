/* Functional ChibiOS PAL (GPIO) stubs for the evaluation harness.
 *
 * palReadPad/palWritePad drive STM32F103 GPIO registers directly
 * so Renode pulse-injector slave can observe/control pin state.
 */
#include "chibios.h"
#include "hw_uart.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* ── STM32F103 GPIO registers ─────────────────────────── */
#define RCC_APB2ENR  (*(volatile uint32_t*)0x40021018)
#define GPIOA_BASE_  0x40010800
#define GPIOB_BASE_  0x40010C00
#define GPIOC_BASE_  0x40011000
#define GPIOD_BASE_  0x40011400
#define GPIOE_BASE_  0x40011800
#define GPIO_CRL(b)  (*(volatile uint32_t*)((b) + 0x00))
#define GPIO_CRH(b)  (*(volatile uint32_t*)((b) + 0x04))
#define GPIO_IDR(b)  (*(volatile uint32_t*)((b) + 0x08))
#define GPIO_ODR(b)  (*(volatile uint32_t*)((b) + 0x0C))
#define GPIO_BSRR(b) (*(volatile uint32_t*)((b) + 0x10))

#define IOPAEN (1U << 2)
#define IOPBEN (1U << 3)
#define IOPCEN (1U << 4)
#define IOPDEN (1U << 5)
#define IOPEEN (1U << 6)

/* ChibiOS port IDs mapped to MMIO bases */
ioportid_t GPIOA = 0, GPIOB = 1, GPIOC = 2, GPIOD = 3, GPIOE = 4;

static uint32_t _port_base(ioportid_t p) {
    switch (p) {
        case 0: return GPIOA_BASE_;
        case 1: return GPIOB_BASE_;
        case 2: return GPIOC_BASE_;
        case 3: return GPIOD_BASE_;
        case 4: return GPIOE_BASE_;
        default: return 0;
    }
}

static void _ensure_clock(ioportid_t p) {
    switch (p) {
        case 0: RCC_APB2ENR |= IOPAEN; break;
        case 1: RCC_APB2ENR |= IOPBEN; break;
        case 2: RCC_APB2ENR |= IOPCEN; break;
        case 3: RCC_APB2ENR |= IOPDEN; break;
        case 4: RCC_APB2ENR |= IOPEEN; break;
    }
}

/* ── PAL (GPIO) stubs (real MMIO) ─────────────────────── */

void palSetPadMode(ioportid_t port, iopadid_t pad, iomode_t mode) {
    _ensure_clock(port);
    uint32_t base = _port_base(port);
    if (!base) return;
    int shift = (pad < 8 ? pad : pad - 8) * 4;
    volatile uint32_t *reg = (pad < 8)
        ? (volatile uint32_t*)(base + 0x00)
        : (volatile uint32_t*)(base + 0x04);
    uint32_t v = *reg;
    v &= ~(0xFU << shift);
    /* OUTPUT modes → push-pull 10MHz (0x1), INPUT → floating (0x4) */
    if (mode == PAL_MODE_OUTPUT_PUSHPULL || mode == PAL_MODE_OUTPUT_OPENDRAIN)
        v |= (0x1U << shift);
    else
        v |= (0x4U << shift);
    *reg = v;
}

uint8_t palReadPad(ioportid_t port, iopadid_t pad) {
    uint32_t base = _port_base(port);
    if (!base) return PAL_LOW;
    return (GPIO_IDR(base) >> pad) & 1U;
}

void palWritePad(ioportid_t port, iopadid_t pad, uint8_t bit) {
    uint32_t base = _port_base(port);
    if (!base) return;
    if (bit) GPIO_BSRR(base) = (1U << pad);
    else     GPIO_BSRR(base) = (1U << (pad + 16));
}

void palSetPad(ioportid_t port, iopadid_t pad) {
    uint32_t base = _port_base(port);
    if (base) GPIO_BSRR(base) = (1U << pad);
}

void palClearPad(ioportid_t port, iopadid_t pad) {
    uint32_t base = _port_base(port);
    if (base) GPIO_BSRR(base) = (1U << (pad + 16));
}

void palTogglePad(ioportid_t port, iopadid_t pad) {
    uint32_t base = _port_base(port);
    if (base) GPIO_ODR(base) ^= (1U << pad);
}

/* Line-based API: line = PAL_LINE(port, pad), as defined in chibios.h. */
void palSetLineMode(ioline_t line, iomode_t mode) {
    palSetPadMode(PAL_PORT(line), PAL_PAD(line), mode);
}
uint8_t palReadLine(ioline_t line) { return palReadPad(PAL_PORT(line), PAL_PAD(line)); }
void palWriteLine(ioline_t line, uint8_t bit) { palWritePad(PAL_PORT(line), PAL_PAD(line), bit); }
void palSetLine(ioline_t line) { palSetPad(PAL_PORT(line), PAL_PAD(line)); }
void palClearLine(ioline_t line) { palClearPad(PAL_PORT(line), PAL_PAD(line)); }
void palToggleLine(ioline_t line) { palTogglePad(PAL_PORT(line), PAL_PAD(line)); }

/* ── I2C dummies ──────────────────────────────────────── */
I2CDriver I2CD1 = {0}, I2CD2 = {0};
void i2cInit(void) {}
void i2cObjectInit(I2CDriver *i) { (void)i; }
msg_t i2cStart(I2CDriver *i, const I2CConfig *c) { (void)i;(void)c; return MSG_OK; }
void i2cStop(I2CDriver *i) { (void)i; }
i2cflags_t i2cGetErrors(I2CDriver *i) { (void)i; return I2C_NO_ERROR; }
msg_t i2cMasterTransmitTimeout(I2CDriver *i, i2caddr_t a, const uint8_t *t, size_t tn,
                               uint8_t *r, size_t rn, sysinterval_t to) {
    (void)i;(void)a;(void)t;(void)tn;(void)to; if(r&&rn) memset(r,0,rn); return MSG_OK;
}
msg_t i2cMasterReceiveTimeout(I2CDriver *i, i2caddr_t a, uint8_t *r, size_t rn,
                              sysinterval_t to) { (void)i;(void)a;(void)to; if(r&&rn) memset(r,0,rn); return MSG_OK; }
void i2cAcquireBus(I2CDriver *i) { (void)i; }
void i2cReleaseBus(I2CDriver *i) { (void)i; }

/* ── SPI dummies ──────────────────────────────────────── */
SPIDriver SPID1 = {0}, SPID2 = {0};
void spiInit(void) {}
void spiObjectInit(SPIDriver *s) { (void)s; }
msg_t spiStart(SPIDriver *s, const SPIConfig *c) { (void)s;(void)c; return MSG_OK; }
void spiStop(SPIDriver *s) { (void)s; }
void spiSelect(SPIDriver *s) { (void)s; }
void spiUnselect(SPIDriver *s) { (void)s; }
void spiSend(SPIDriver *s, size_t n, const void *t) { (void)s;(void)n;(void)t; }
void spiReceive(SPIDriver *s, size_t n, void *r) { (void)s;(void)n; if(r&&n) memset(r,0,n); }
void spiExchange(SPIDriver *s, size_t n, const void *t, void *r) { (void)s;(void)n;(void)t; if(r&&n) memset(r,0,n); }

/* ── Serial dummies ───────────────────────────────────── */
SerialDriver SD2 = {{0},0,0}, SD3 = {{0},0,0};
void sdInit(void) {}
void sdObjectInit(SerialDriver *s) { (void)s; }
msg_t sdStart(SerialDriver *s, const SerialConfig *c) { (void)s;(void)c; return MSG_OK; }
void sdStop(SerialDriver *s) { (void)s; }
size_t sdWrite(SerialDriver *s, const uint8_t *b, size_t n) { (void)s;(void)b; return n; }
size_t sdRead(SerialDriver *s, uint8_t *b, size_t n) { (void)s; if(b&&n) memset(b,0,n); return n; }
size_t sdWriteTimeout(SerialDriver *s, const uint8_t *b, size_t n, sysinterval_t t) { (void)s;(void)b;(void)t; return n; }
size_t sdReadTimeout(SerialDriver *s, uint8_t *b, size_t n, sysinterval_t t) { (void)s;(void)t; if(b&&n) memset(b,0,n); return n; }

/* ── Thread / Mutex / Sem / VTimer ────────────────────── */
thread_t *chThdCreateStatic(stkline_t *w, size_t ws, tprio_t p, tfunc_t f, void *a) { (void)w;(void)ws;(void)p;(void)f;(void)a; return 0; }
void chThdSleep(sysinterval_t t) { (void)t; }
void osalThreadSleep(sysinterval_t t) { (void)t; }
void chMtxObjectInit(mutex_t *m) { (void)m; }
void chMtxLock(mutex_t *m) { (void)m; }
bool chMtxTryLock(mutex_t *m) { (void)m; return true; }
void chMtxUnlock(mutex_t *m) { (void)m; }
void chSemObjectInit(semaphore_t *s, cnt_t n) { (void)s;(void)n; }
msg_t chSemWait(semaphore_t *s) { (void)s; return MSG_OK; }
msg_t chSemWaitTimeout(semaphore_t *s, sysinterval_t t) { (void)s;(void)t; return MSG_OK; }
void chSemSignal(semaphore_t *s) { (void)s; }
void chVTObjectInit(virtual_timer_t *v) { (void)v; }
void chVTSet(virtual_timer_t *v, sysinterval_t d, vtfunc_t f, void *p) { (void)v;(void)d;(void)f;(void)p; }
void chVTReset(virtual_timer_t *v) { (void)v; }

BaseSequentialStream SD1 = {0};
int chprintf(BaseSequentialStream *c, const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}
int chsnprintf(char *s, size_t sz, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); int n = vsnprintf(s, sz, fmt, ap); va_end(ap); return n;
}
/* ── Microsecond delay ── */
void chSysPolledDelayX(rtcnt_t cycles) { (void)cycles; }
rtcnt_t chSysGetRealtimeCounterX(void) { static rtcnt_t t = 0; return t += 72; }

/* ── Device-specific delay_us (GPIO sensor drivers) ── */
void dht22_delay_us(uint32_t us) { (void)us; }
void ds18b20_delay_us(uint32_t us) { (void)us; }
void hcsr04_delay_us(uint32_t us) { (void)us; }
uint32_t hcsr04_get_us_tick(void) { static uint32_t t = 0; return t += 1; }

void halInit(void) {}
void chSysInit(void) {}
void *chHeapAlloc(void *h, size_t s) { (void)h;(void)s; return 0; }
void chHeapFree(void *p) { (void)p; }

__attribute__((weak)) int main(void) { return 0; }
