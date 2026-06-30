/* Functional OpenHarmony LiteOS-M UART stubs — UartWrite/UartRead route through hw_uart_bus.h
 *
 * HDF UART API: UartOpen/Close/Write/Read/SetBaud/SetAttribute.
 */
#include "openharmony_liteosm.h"
#include "uart_if.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ── UART stubs (real STM32 USART1 via hw_uart_bus.h) ── */

DevHandle UartOpen(uint32_t port) {
    (void)port;
    hw_uart_bus_init();
    return (DevHandle)1;
}

void UartClose(DevHandle handle) { (void)handle; }

int32_t UartWrite(DevHandle handle, uint8_t *data, uint32_t size) {
    (void)handle;
    for (uint32_t i = 0; i < size; i++)
        hw_uart_bus_write_byte(data[i]);
    return HDF_SUCCESS;
}

int32_t UartRead(DevHandle handle, uint8_t *data, uint32_t size) {
    (void)handle;
    for (uint32_t i = 0; i < size; i++) {
        if (hw_uart_bus_read_byte(&data[i]) != 0)
            return (int32_t)i;
    }
    return (int32_t)size;
}

int32_t UartSetBaud(DevHandle handle, uint32_t baud) { (void)handle;(void)baud; return HDF_SUCCESS; }
int32_t UartGetBaud(DevHandle handle, uint32_t *baud) { (void)handle; if(baud) *baud=9600; return HDF_SUCCESS; }
int32_t UartSetAttribute(DevHandle handle, struct UartAttribute *a) { (void)handle;(void)a; return HDF_SUCCESS; }
int32_t UartGetAttribute(DevHandle handle, struct UartAttribute *a) { (void)handle;(void)a; return HDF_SUCCESS; }
int32_t UartSetTransMode(DevHandle handle, uint32_t mode) { (void)handle;(void)mode; return HDF_SUCCESS; }

/* ── I2C dummies ──────────────────────────────────────── */
DevHandle I2cOpen(int16_t number) { (void)number; return (DevHandle)1; }
void I2cClose(DevHandle handle) { (void)handle; }
int32_t I2cTransfer(DevHandle handle, struct I2cMsg *msgs, int16_t count) {
    (void)handle;
    for (int16_t i = 0; i < count; i++) {
        if ((msgs[i].flags & I2C_FLAG_READ) && msgs[i].buf && msgs[i].len > 0)
            memset(msgs[i].buf, 0, msgs[i].len);
    }
    return count;
}

/* ── SPI dummies ──────────────────────────────────────── */
DevHandle SpiOpen(struct SpiDevInfo *info) { (void)info; return (DevHandle)1; }
void SpiClose(DevHandle handle) { (void)handle; }
int32_t SpiTransfer(DevHandle handle, struct SpiMsg *msgs, uint32_t count) { (void)handle;(void)msgs;(void)count; return HDF_SUCCESS; }
int32_t SpiRead(DevHandle handle, uint8_t *buf, uint32_t len) { (void)handle; if(buf&&len) memset(buf,0,len); return HDF_SUCCESS; }
int32_t SpiWrite(DevHandle handle, uint8_t *buf, uint32_t len) { (void)handle;(void)buf;(void)len; return HDF_SUCCESS; }
int32_t SpiSetCfg(DevHandle handle, struct SpiCfg *cfg) { (void)handle;(void)cfg; return HDF_SUCCESS; }
int32_t SpiGetCfg(DevHandle handle, struct SpiCfg *cfg) { (void)handle;(void)cfg; return HDF_SUCCESS; }

/* ── GPIO dummies ─────────────────────────────────────── */
int32_t GpioSetDir(uint16_t gpio, uint16_t dir) { (void)gpio;(void)dir; return HDF_SUCCESS; }
int32_t GpioGetDir(uint16_t gpio, uint16_t *dir) { (void)gpio; if(dir) *dir=0; return HDF_SUCCESS; }
int32_t GpioRead(uint16_t gpio, uint16_t *val) { (void)gpio; if(val) *val=0; return HDF_SUCCESS; }
int32_t GpioWrite(uint16_t gpio, uint16_t val) { (void)gpio;(void)val; return HDF_SUCCESS; }
int32_t GpioSetIrq(uint16_t gpio, uint16_t mode, GpioIrqFunc func, void *arg) { (void)gpio;(void)mode;(void)func;(void)arg; return HDF_SUCCESS; }
int32_t GpioUnsetIrq(uint16_t gpio, void *arg) { (void)gpio;(void)arg; return HDF_SUCCESS; }
int32_t GpioEnableIrq(uint16_t gpio) { (void)gpio; return HDF_SUCCESS; }
int32_t GpioDisableIrq(uint16_t gpio) { (void)gpio; return HDF_SUCCESS; }

/* ── OSAL ─────────────────────────────────────────────── */
void OsalMSleep(uint32_t ms) { (void)ms; }
void OsalSleep(uint32_t sec) { (void)sec; }
void OsalUDelay(uint32_t us) { (void)us; }
void OsalMDelay(uint32_t ms) { (void)ms; }
uint64_t OsalGetSysTimeMs(void) { return 0; }
int32_t OsalGetTime(OsalTimespec *t) { (void)t; return HDF_SUCCESS; }

int32_t OsalMutexInit(OsalMutex *m) { (void)m; return HDF_SUCCESS; }
int32_t OsalMutexDestroy(OsalMutex *m) { (void)m; return HDF_SUCCESS; }
int32_t OsalMutexLock(OsalMutex *m) { (void)m; return HDF_SUCCESS; }
int32_t OsalMutexTimedLock(OsalMutex *m, uint32_t ms) { (void)m;(void)ms; return HDF_SUCCESS; }
int32_t OsalMutexUnlock(OsalMutex *m) { (void)m; return HDF_SUCCESS; }
int32_t OsalSpinInit(OsalSpinlock *l) { (void)l; return HDF_SUCCESS; }
int32_t OsalSpinDestroy(OsalSpinlock *l) { (void)l; return HDF_SUCCESS; }
int32_t OsalSpinLock(OsalSpinlock *l) { (void)l; return HDF_SUCCESS; }
int32_t OsalSpinUnlock(OsalSpinlock *l) { (void)l; return HDF_SUCCESS; }
int32_t OsalSemInit(OsalSem *s, uint32_t v) { (void)s;(void)v; return HDF_SUCCESS; }
int32_t OsalSemWait(OsalSem *s, uint32_t ms) { (void)s;(void)ms; return HDF_SUCCESS; }
int32_t OsalSemPost(OsalSem *s) { (void)s; return HDF_SUCCESS; }
int32_t OsalSemDestroy(OsalSem *s) { (void)s; return HDF_SUCCESS; }
int32_t OsalThreadCreate(OsalThread *t, int (*entry)(void*), void *para) { (void)t;(void)entry;(void)para; return HDF_SUCCESS; }
int32_t OsalThreadStart(OsalThread *t, const OsalThreadParam *p) { (void)t;(void)p; return HDF_SUCCESS; }
int32_t OsalThreadDestroy(OsalThread *t) { (void)t; return HDF_SUCCESS; }

void *OsalMemAlloc(size_t size) { return malloc(size); }
void *OsalMemCalloc(size_t size) { return calloc(1, size); }
void OsalMemFree(void *mem) { free(mem); }

int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}
__attribute__((weak)) int main(void) { return 0; }
