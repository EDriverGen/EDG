/*
 * ChibiOS/RT unified stub header for DriverGen stub compilation.
 * Covers: HAL I2C, PAL GPIO, RT kernel (threads, mutex, semaphore), chprintf.
 */
#ifndef CHIBIOS_STUB_H
#define CHIBIOS_STUB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Base types ─────────────────────────────────────────── */
typedef int32_t  msg_t;
typedef uint32_t systime_t;
typedef uint32_t sysinterval_t;
typedef uint32_t eventflags_t;
typedef int32_t  cnt_t;
typedef uint8_t  tprio_t;
typedef void (*tfunc_t)(void *);

#define MSG_OK       (msg_t)0
#define MSG_TIMEOUT  (msg_t)-1
#define MSG_RESET    (msg_t)-2

#define HAL_RET_SUCCESS       MSG_OK
#define HAL_RET_CONFIG_ERROR  MSG_RESET

#define TIME_INFINITE  ((sysinterval_t)0xFFFFFFFFU)

/* ---- Thread sleep (chthreads.h) ---- */
static inline void chThdSleepMilliseconds(uint32_t msec) { (void)msec; }
#define TIME_IMMEDIATE ((sysinterval_t)0U)
#define TIME_MS2I(msec)  ((sysinterval_t)(msec))
#define MS2ST(msec)       ((systime_t)(msec))
#define TIME_US2I(usec)  ((sysinterval_t)((usec) / 1000U + 1U))
#define TIME_S2I(sec)    ((sysinterval_t)((sec) * 1000U))

#define OSAL_MS2I(msec)  TIME_MS2I(msec)
#define OSAL_US2I(usec)  TIME_US2I(usec)

/* ── I2C ────────────────────────────────────────────────── */
typedef uint16_t i2caddr_t;
typedef uint32_t i2cflags_t;

#define I2C_NO_ERROR          0x00
#define I2C_BUS_ERROR         0x01
#define I2C_ARBITRATION_LOST  0x02
#define I2C_ACK_FAILURE       0x04
#define I2C_OVERRUN           0x08
#define I2C_PEC_ERROR         0x10
#define I2C_TIMEOUT           0x20
#define I2C_SMB_ALERT         0x40

typedef enum {
    I2C_UNINIT = 0,
    I2C_STOP   = 1,
    I2C_READY  = 2,
    I2C_ACTIVE_TX = 3,
    I2C_ACTIVE_RX = 4,
    I2C_LOCKED = 5,
} i2cstate_t;

typedef enum {
    OPMODE_I2C       = 1,
    OPMODE_SMBUS_DEVICE = 2,
    OPMODE_SMBUS_HOST   = 3,
} i2copmode_t;

typedef enum {
    STD_DUTY_CYCLE    = 1,
    FAST_DUTY_CYCLE_2 = 2,
    FAST_DUTY_CYCLE_16_9 = 3,
} i2cdutycycle_t;

typedef struct {
    i2copmode_t    op_mode;
    uint32_t       clock_speed;
    i2cdutycycle_t duty_cycle;
} I2CConfig;

typedef struct {
    i2cstate_t   state;
    i2cflags_t   errors;
    const I2CConfig *config;
} I2CDriver;

extern I2CDriver I2CD1;
extern I2CDriver I2CD2;

void i2cInit(void);
void i2cObjectInit(I2CDriver *i2cp);
msg_t i2cStart(I2CDriver *i2cp, const I2CConfig *config);
void i2cStop(I2CDriver *i2cp);
i2cflags_t i2cGetErrors(I2CDriver *i2cp);

msg_t i2cMasterTransmitTimeout(I2CDriver *i2cp, i2caddr_t addr,
                               const uint8_t *txbuf, size_t txbytes,
                               uint8_t *rxbuf, size_t rxbytes,
                               sysinterval_t timeout);

msg_t i2cMasterReceiveTimeout(I2CDriver *i2cp, i2caddr_t addr,
                              uint8_t *rxbuf, size_t rxbytes,
                              sysinterval_t timeout);

#define i2cMasterTransmit(i2cp, addr, txbuf, txbytes, rxbuf, rxbytes) \
    i2cMasterTransmitTimeout(i2cp, addr, txbuf, txbytes, rxbuf, rxbytes, TIME_INFINITE)

#define i2cMasterReceive(i2cp, addr, rxbuf, rxbytes) \
    i2cMasterReceiveTimeout(i2cp, addr, rxbuf, rxbytes, TIME_INFINITE)

void i2cAcquireBus(I2CDriver *i2cp);
void i2cReleaseBus(I2CDriver *i2cp);

/* ── Streams / chprintf (forward declaration) ───────────── */
typedef struct {
    uint8_t dummy;
} BaseSequentialStream;

/* ── SPI ────────────────────────────────────────────────── */
typedef enum {
    SPI_UNINIT = 0,
    SPI_STOP   = 1,
    SPI_READY  = 2,
    SPI_ACTIVE = 3,
} spistate_t;

typedef struct {
    uint16_t ssport;
    uint16_t sspad;
    uint16_t cr1;
    uint16_t cr2;
} SPIConfig;

typedef struct {
    spistate_t state;
    const SPIConfig *config;
} SPIDriver;

extern SPIDriver SPID1;
extern SPIDriver SPID2;

void spiInit(void);
void spiObjectInit(SPIDriver *spip);
msg_t spiStart(SPIDriver *spip, const SPIConfig *config);
void spiStop(SPIDriver *spip);
void spiSelect(SPIDriver *spip);
void spiUnselect(SPIDriver *spip);
void spiSend(SPIDriver *spip, size_t n, const void *txbuf);
void spiReceive(SPIDriver *spip, size_t n, void *rxbuf);
void spiExchange(SPIDriver *spip, size_t n, const void *txbuf, void *rxbuf);
void spiAcquireBus(SPIDriver *spip);
void spiReleaseBus(SPIDriver *spip);
msg_t spiStartExchangeI(SPIDriver *spip, size_t n, const void *txbuf, void *rxbuf);

/* ── Serial ─────────────────────────────────────────────── */
typedef struct {
    uint32_t speed;
    uint32_t cr1;
    uint32_t cr2;
    uint32_t cr3;
} SerialConfig;

typedef struct {
    BaseSequentialStream vmt_bss;
    uint8_t state;
    const SerialConfig *config;
} SerialDriver;

extern SerialDriver SD2;
extern SerialDriver SD3;

void sdInit(void);
void sdObjectInit(SerialDriver *sdp);
msg_t sdStart(SerialDriver *sdp, const SerialConfig *config);
void sdStop(SerialDriver *sdp);
size_t sdWrite(SerialDriver *sdp, const uint8_t *buf, size_t n);
size_t sdRead(SerialDriver *sdp, uint8_t *buf, size_t n);
size_t sdWriteTimeout(SerialDriver *sdp, const uint8_t *buf, size_t n, sysinterval_t timeout);
size_t sdReadTimeout(SerialDriver *sdp, uint8_t *buf, size_t n, sysinterval_t timeout);
msg_t sdPut(SerialDriver *sdp, uint8_t b);
msg_t sdGet(SerialDriver *sdp);
size_t sdAsynchronousRead(SerialDriver *sdp, uint8_t *buf, size_t n);

/* Stream-style write/read for BaseSequentialStream */
size_t streamWrite(void *ip, const uint8_t *bp, size_t n);
size_t streamRead(void *ip, uint8_t *bp, size_t n);

/* ── Polled delay / timing ──────────────────────────────── */
typedef uint32_t rtcnt_t;
#define STM32_HCLK  72000000U

rtcnt_t chSysGetRealtimeCounterX(void);
void chSysPolledDelayX(rtcnt_t cycles);

#define US2RTC(freq, us)  ((rtcnt_t)((uint64_t)(freq) * (us) / 1000000U))
#define RTC2US(freq, n)   ((uint32_t)((uint64_t)(n) * 1000000U / (freq)))

/* ── PAL (GPIO) ─────────────────────────────────────────── */
typedef uint32_t ioportid_t;
typedef uint32_t ioportmask_t;
typedef uint32_t iopadid_t;
typedef uint32_t iomode_t;
typedef uint32_t ioline_t;

#define PAL_MODE_INPUT              2U
#define PAL_MODE_INPUT_PULLUP       3U
#define PAL_MODE_INPUT_PULLDOWN     4U
#define PAL_MODE_INPUT_ANALOG       5U
#define PAL_MODE_OUTPUT_PUSHPULL    6U
#define PAL_MODE_OUTPUT_OPENDRAIN   7U
#define PAL_LOW   0U
#define PAL_HIGH  1U

#define PAL_LINE(port, pad) ((ioline_t)((uint32_t)(port) << 16 | (uint32_t)(pad)))
#define PAL_PORT(line)     ((ioportid_t)(((uint32_t)(line) >> 16) & 0xFFU))
#define PAL_PAD(line)      ((iopadid_t)((uint32_t)(line) & 0xFFU))

extern ioportid_t GPIOA;
extern ioportid_t GPIOB;
extern ioportid_t GPIOC;
extern ioportid_t GPIOD;
extern ioportid_t GPIOE;

uint8_t palReadPad(ioportid_t port, iopadid_t pad);
void palWritePad(ioportid_t port, iopadid_t pad, uint8_t bit);
void palSetPad(ioportid_t port, iopadid_t pad);
void palClearPad(ioportid_t port, iopadid_t pad);
void palTogglePad(ioportid_t port, iopadid_t pad);
void palSetPadMode(ioportid_t port, iopadid_t pad, iomode_t mode);

uint8_t palReadLine(ioline_t line);
void palWriteLine(ioline_t line, uint8_t bit);
void palSetLine(ioline_t line);
void palClearLine(ioline_t line);
void palToggleLine(ioline_t line);
void palSetLineMode(ioline_t line, iomode_t mode);

/* ── Thread ─────────────────────────────────────────────── */
typedef struct ch_thread thread_t;

typedef struct {
    uint8_t dummy;
} stkalign_t;

typedef stkalign_t stkline_t;

#define THD_WORKING_AREA(s, n) stkline_t s[(n) / sizeof(stkline_t) + 1]
#define THD_FUNCTION(tname, arg) void tname(void *arg)

thread_t *chThdCreateStatic(stkline_t *wbase, size_t wsize,
                            tprio_t prio, tfunc_t func, void *arg);
void chThdSleep(sysinterval_t time);
#define chThdSleepSeconds(sec)       chThdSleep(TIME_S2I(sec))
#define chThdSleepMilliseconds(msec) chThdSleep(TIME_MS2I(msec))
#define chThdSleepMicroseconds(usec) chThdSleep(TIME_US2I(usec))

void osalThreadSleep(sysinterval_t time);
#define osalThreadSleepMilliseconds(msecs) osalThreadSleep(OSAL_MS2I(msecs))
#define osalThreadSleepMicroseconds(usecs) osalThreadSleep(OSAL_US2I(usecs))

/* ── Mutex ──────────────────────────────────────────────── */
typedef struct {
    uint8_t dummy;
} mutex_t;

#define MUTEX_DECL(name) mutex_t name = {0}

void chMtxObjectInit(mutex_t *mp);
void chMtxLock(mutex_t *mp);
bool chMtxTryLock(mutex_t *mp);
void chMtxUnlock(mutex_t *mp);

/* ── Semaphore ──────────────────────────────────────────── */
typedef struct {
    cnt_t cnt;
} semaphore_t;

void chSemObjectInit(semaphore_t *sp, cnt_t n);
msg_t chSemWait(semaphore_t *sp);
msg_t chSemWaitTimeout(semaphore_t *sp, sysinterval_t timeout);
void chSemSignal(semaphore_t *sp);

/* ── Virtual Timer ──────────────────────────────────────── */
typedef struct {
    uint8_t dummy;
} virtual_timer_t;

typedef void (*vtfunc_t)(virtual_timer_t *vtp, void *par);

void chVTObjectInit(virtual_timer_t *vtp);
void chVTSet(virtual_timer_t *vtp, sysinterval_t delay, vtfunc_t vtfunc, void *par);
void chVTReset(virtual_timer_t *vtp);

/* ── Streams / chprintf ─────────────────────────────────── */
/* BaseSequentialStream already forward-declared above SPI section */

extern BaseSequentialStream SD1;

int chprintf(BaseSequentialStream *chp, const char *fmt, ...);
int chsnprintf(char *str, size_t size, const char *fmt, ...);

/* ── HAL init ───────────────────────────────────────────── */
void halInit(void);
void chSysInit(void);

/* ── Memory management ──────────────────────────────────── */
void *chHeapAlloc(void *hp, size_t size);
void chHeapFree(void *p);

/* ── Convenience includes ───────────────────────────────── */
/* These are provided so that #include "ch.h" or "hal.h" resolve. */

#ifdef __cplusplus
}
#endif

#endif /* CHIBIOS_STUB_H */
