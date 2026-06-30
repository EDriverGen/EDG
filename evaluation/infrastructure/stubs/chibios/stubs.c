/* ChibiOS stub implementations for DriverGen syntax/link checking. */
#include "chibios.h"
#include <string.h>
#include <stdarg.h>

/* ── I2C driver instances ─────────────────────────────── */
I2CDriver I2CD1 = {0};
I2CDriver I2CD2 = {0};

void i2cInit(void) {}
void i2cObjectInit(I2CDriver *i2cp) { (void)i2cp; }
msg_t i2cStart(I2CDriver *i2cp, const I2CConfig *config) { (void)i2cp; (void)config; return MSG_OK; }
void i2cStop(I2CDriver *i2cp) { (void)i2cp; }
i2cflags_t i2cGetErrors(I2CDriver *i2cp) { (void)i2cp; return I2C_NO_ERROR; }

msg_t i2cMasterTransmitTimeout(I2CDriver *i2cp, i2caddr_t addr,
                               const uint8_t *txbuf, size_t txbytes,
                               uint8_t *rxbuf, size_t rxbytes,
                               sysinterval_t timeout) {
    (void)i2cp; (void)addr; (void)txbuf; (void)txbytes; (void)timeout;
    if (rxbuf && rxbytes > 0) memset(rxbuf, 0x19, rxbytes);
    return MSG_OK;
}

msg_t i2cMasterReceiveTimeout(I2CDriver *i2cp, i2caddr_t addr,
                              uint8_t *rxbuf, size_t rxbytes,
                              sysinterval_t timeout) {
    (void)i2cp; (void)addr; (void)timeout;
    if (rxbuf && rxbytes > 0) memset(rxbuf, 0x19, rxbytes);
    return MSG_OK;
}

void i2cAcquireBus(I2CDriver *i2cp) { (void)i2cp; }
void i2cReleaseBus(I2CDriver *i2cp) { (void)i2cp; }

/* ── PAL (GPIO) ───────────────────────────────────────── */
ioportid_t GPIOA = 0;
ioportid_t GPIOB = 1;
ioportid_t GPIOC = 2;
ioportid_t GPIOD = 3;
ioportid_t GPIOE = 4;

uint8_t palReadPad(ioportid_t port, iopadid_t pad) { (void)port; (void)pad; return PAL_LOW; }
void palWritePad(ioportid_t port, iopadid_t pad, uint8_t bit) { (void)port; (void)pad; (void)bit; }
void palSetPad(ioportid_t port, iopadid_t pad) { (void)port; (void)pad; }
void palClearPad(ioportid_t port, iopadid_t pad) { (void)port; (void)pad; }
void palTogglePad(ioportid_t port, iopadid_t pad) { (void)port; (void)pad; }
void palSetPadMode(ioportid_t port, iopadid_t pad, iomode_t mode) { (void)port; (void)pad; (void)mode; }

uint8_t palReadLine(ioline_t line) { (void)line; return PAL_LOW; }
void palWriteLine(ioline_t line, uint8_t bit) { (void)line; (void)bit; }
void palSetLine(ioline_t line) { (void)line; }
void palClearLine(ioline_t line) { (void)line; }
void palToggleLine(ioline_t line) { (void)line; }
void palSetLineMode(ioline_t line, iomode_t mode) { (void)line; (void)mode; }

/* ── Thread ───────────────────────────────────────────── */
thread_t *chThdCreateStatic(stkline_t *wbase, size_t wsize,
                            tprio_t prio, tfunc_t func, void *arg) {
    (void)wbase; (void)wsize; (void)prio; (void)func; (void)arg;
    return (thread_t *)0;
}
void chThdSleep(sysinterval_t time) { (void)time; }
void osalThreadSleep(sysinterval_t time) { (void)time; }

/* ── Mutex ────────────────────────────────────────────── */
void chMtxObjectInit(mutex_t *mp) { (void)mp; }
void chMtxLock(mutex_t *mp) { (void)mp; }
bool chMtxTryLock(mutex_t *mp) { (void)mp; return true; }
void chMtxUnlock(mutex_t *mp) { (void)mp; }

/* ── Semaphore ────────────────────────────────────────── */
void chSemObjectInit(semaphore_t *sp, cnt_t n) { (void)sp; (void)n; }
msg_t chSemWait(semaphore_t *sp) { (void)sp; return MSG_OK; }
msg_t chSemWaitTimeout(semaphore_t *sp, sysinterval_t timeout) { (void)sp; (void)timeout; return MSG_OK; }
void chSemSignal(semaphore_t *sp) { (void)sp; }

/* ── Virtual Timer ────────────────────────────────────── */
void chVTObjectInit(virtual_timer_t *vtp) { (void)vtp; }
void chVTSet(virtual_timer_t *vtp, sysinterval_t delay, vtfunc_t vtfunc, void *par) {
    (void)vtp; (void)delay; (void)vtfunc; (void)par;
}
void chVTReset(virtual_timer_t *vtp) { (void)vtp; }

/* ── Streams / chprintf ───────────────────────────────── */
BaseSequentialStream SD1 = {0};

int chprintf(BaseSequentialStream *chp, const char *fmt, ...) {
    (void)chp; (void)fmt;
    return 0;
}
int chsnprintf(char *str, size_t size, const char *fmt, ...) {
    (void)str; (void)size; (void)fmt;
    return 0;
}

/* ── HAL init ─────────────────────────────────────────── */
void halInit(void) {}
void chSysInit(void) {}

/* ── Memory ───────────────────────────────────────────── */
void *chHeapAlloc(void *hp, size_t size) { (void)hp; (void)size; return (void *)0; }
void chHeapFree(void *p) { (void)p; }

/* ── Entry point ──────────────────────────────────────── */
__attribute__((weak)) int main(void) { return 0; }
