/* Functional ChibiOS I2C stubs for the evaluation harness.
 *
 * i2cMasterTransmitTimeout / i2cMasterReceiveTimeout route through
 * hw_i2c.h (I2C1 registers) so the driver really drives STM32 I2C1 in Renode.
 * Note: ChibiOS uses 7-bit addresses directly (no shift needed).
 */
#include "chibios.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* ── I2C driver instances ─────────────────────────────── */
I2CDriver I2CD1 = {0};
I2CDriver I2CD2 = {0};

void i2cInit(void) {}
void i2cObjectInit(I2CDriver *i2cp) { (void)i2cp; }
msg_t i2cStart(I2CDriver *i2cp, const I2CConfig *config) {
    (void)i2cp; (void)config; hw_i2c1_init(); return MSG_OK;
}
void i2cStop(I2CDriver *i2cp) { (void)i2cp; }
i2cflags_t i2cGetErrors(I2CDriver *i2cp) { (void)i2cp; return I2C_NO_ERROR; }

msg_t i2cMasterTransmitTimeout(I2CDriver *i2cp, i2caddr_t addr,
                               const uint8_t *txbuf, size_t txbytes,
                               uint8_t *rxbuf, size_t rxbytes,
                               sysinterval_t timeout) {
    (void)i2cp; (void)timeout;
    if (rxbytes > 0 && rxbuf) {
        /* Write-then-read: send txbuf as register address(es), read rxbuf */
        return hw_i2c_write_read(0, (uint8_t)addr, txbuf, (uint16_t)txbytes,
                                 rxbuf, (uint16_t)rxbytes) == 0 ? MSG_OK : MSG_TIMEOUT;
    }
    /* Write only */
    return hw_i2c_write(0, (uint8_t)addr, txbuf, (uint16_t)txbytes) == 0 ? MSG_OK : MSG_TIMEOUT;
}

msg_t i2cMasterReceiveTimeout(I2CDriver *i2cp, i2caddr_t addr,
                              uint8_t *rxbuf, size_t rxbytes,
                              sysinterval_t timeout) {
    (void)i2cp; (void)timeout;
    return hw_i2c_read(0, (uint8_t)addr, rxbuf, (uint16_t)rxbytes) == 0 ? MSG_OK : MSG_TIMEOUT;
}

void i2cAcquireBus(I2CDriver *i2cp) { (void)i2cp; }
void i2cReleaseBus(I2CDriver *i2cp) { (void)i2cp; }

/* ── PAL (GPIO) dummies ───────────────────────────────── */
ioportid_t GPIOA = 0, GPIOB = 1, GPIOC = 2, GPIOD = 3, GPIOE = 4;
uint8_t palReadPad(ioportid_t p, iopadid_t d) { (void)p;(void)d; return PAL_LOW; }
void palWritePad(ioportid_t p, iopadid_t d, uint8_t b) { (void)p;(void)d;(void)b; }
void palSetPad(ioportid_t p, iopadid_t d) { (void)p;(void)d; }
void palClearPad(ioportid_t p, iopadid_t d) { (void)p;(void)d; }
void palTogglePad(ioportid_t p, iopadid_t d) { (void)p;(void)d; }
void palSetPadMode(ioportid_t p, iopadid_t d, iomode_t m) { (void)p;(void)d;(void)m; }
uint8_t palReadLine(ioline_t l) { (void)l; return PAL_LOW; }
void palWriteLine(ioline_t l, uint8_t b) { (void)l;(void)b; }
void palSetLine(ioline_t l) { (void)l; }
void palClearLine(ioline_t l) { (void)l; }
void palToggleLine(ioline_t l) { (void)l; }
void palSetLineMode(ioline_t l, iomode_t m) { (void)l;(void)m; }

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

/* ── Streams / printf ─────────────────────────────────── */
BaseSequentialStream SD1 = {0};
int chprintf(BaseSequentialStream *c, const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}
int chsnprintf(char *s, size_t sz, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(s, sz, fmt, ap); va_end(ap);
    return n;
}

/* ── HAL / sys init ───────────────────────────────────── */
void halInit(void) {}
void chSysInit(void) {}
void *chHeapAlloc(void *h, size_t s) { (void)h;(void)s; return 0; }
void chHeapFree(void *p) { (void)p; }

__attribute__((weak)) int main(void) { return 0; }
