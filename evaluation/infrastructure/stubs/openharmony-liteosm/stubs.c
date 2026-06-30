/* OpenHarmony LiteOS-M + HDF stub implementations */
#include "openharmony_liteosm.h"
#include <stdlib.h>

/* I2C */
DevHandle I2cOpen(int16_t number) { (void)number; return (DevHandle)1; }
void I2cClose(DevHandle handle) { (void)handle; }
int32_t I2cTransfer(DevHandle handle, struct I2cMsg *msgs, int16_t count) {
    (void)handle;
    for (int16_t i = 0; i < count; i++) {
        if (msgs[i].flags & I2C_FLAG_READ) {
            if (msgs[i].buf && msgs[i].len > 0)
                memset(msgs[i].buf, 0x19, msgs[i].len);
        }
    }
    return count;
}

/* GPIO */
int32_t GpioSetDir(uint16_t gpio, uint16_t dir) { (void)gpio;(void)dir; return HDF_SUCCESS; }
int32_t GpioGetDir(uint16_t gpio, uint16_t *dir) { (void)gpio; if(dir) *dir=0; return HDF_SUCCESS; }
int32_t GpioRead(uint16_t gpio, uint16_t *val) { (void)gpio; if(val) *val=0; return HDF_SUCCESS; }
int32_t GpioWrite(uint16_t gpio, uint16_t val) { (void)gpio;(void)val; return HDF_SUCCESS; }
int32_t GpioSetIrq(uint16_t gpio, uint16_t mode, GpioIrqFunc func, void *arg) {
    (void)gpio;(void)mode;(void)func;(void)arg; return HDF_SUCCESS;
}
int32_t GpioUnsetIrq(uint16_t gpio, void *arg) { (void)gpio;(void)arg; return HDF_SUCCESS; }
int32_t GpioEnableIrq(uint16_t gpio) { (void)gpio; return HDF_SUCCESS; }
int32_t GpioDisableIrq(uint16_t gpio) { (void)gpio; return HDF_SUCCESS; }

/* OSAL timing */
void OsalMSleep(uint32_t ms) { (void)ms; }
void OsalSleep(uint32_t sec) { (void)sec; }
void OsalUDelay(uint32_t us) { (void)us; }
void OsalMDelay(uint32_t ms) { (void)ms; }
uint64_t OsalGetSysTimeMs(void) { return 0; }

/* OSAL mutex */
int32_t OsalMutexInit(OsalMutex *m) { (void)m; return HDF_SUCCESS; }
int32_t OsalMutexDestroy(OsalMutex *m) { (void)m; return HDF_SUCCESS; }
int32_t OsalMutexLock(OsalMutex *m) { (void)m; return HDF_SUCCESS; }
int32_t OsalMutexTimedLock(OsalMutex *m, uint32_t ms) { (void)m;(void)ms; return HDF_SUCCESS; }
int32_t OsalMutexUnlock(OsalMutex *m) { (void)m; return HDF_SUCCESS; }

/* OSAL spinlock */
int32_t OsalSpinInit(OsalSpinlock *l) { (void)l; return HDF_SUCCESS; }
int32_t OsalSpinDestroy(OsalSpinlock *l) { (void)l; return HDF_SUCCESS; }
int32_t OsalSpinLock(OsalSpinlock *l) { (void)l; return HDF_SUCCESS; }
int32_t OsalSpinUnlock(OsalSpinlock *l) { (void)l; return HDF_SUCCESS; }

/* OSAL semaphore */
int32_t OsalSemInit(OsalSem *s, uint32_t v) { (void)s;(void)v; return HDF_SUCCESS; }
int32_t OsalSemWait(OsalSem *s, uint32_t ms) { (void)s;(void)ms; return HDF_SUCCESS; }
int32_t OsalSemPost(OsalSem *s) { (void)s; return HDF_SUCCESS; }
int32_t OsalSemDestroy(OsalSem *s) { (void)s; return HDF_SUCCESS; }

/* OSAL thread */
int32_t OsalThreadCreate(OsalThread *t, int (*entry)(void*), void *para) {
    (void)t;(void)entry;(void)para; return HDF_SUCCESS;
}
int32_t OsalThreadStart(OsalThread *t, const OsalThreadParam *p) {
    (void)t;(void)p; return HDF_SUCCESS;
}
int32_t OsalThreadDestroy(OsalThread *t) { (void)t; return HDF_SUCCESS; }

/* OSAL memory */
void *OsalMemAlloc(size_t size) { return malloc(size); }
void *OsalMemCalloc(size_t size) { return calloc(1, size); }
void OsalMemFree(void *mem) { free(mem); }

__attribute__((weak)) int main(void) { return 0; }
