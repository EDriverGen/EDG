/*
 * OpenHarmony LiteOS-M + HDF unified stub for syntax-only compilation tests.
 * Covers: HDF I2C (I2cOpen/I2cTransfer/I2cClose), GPIO (GpioSetDir/Read/Write),
 *         OSAL timing (OsalMSleep/OsalUDelay), HDF device driver model.
 */
#ifndef __OPENHARMONY_LITEOSM_STUB_H__
#define __OPENHARMONY_LITEOSM_STUB_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- HDF return codes ---------- */
#define HDF_SUCCESS         0
#define HDF_FAILURE         (-1)
#define HDF_ERR_NOT_SUPPORT (-2)
#define HDF_ERR_INVALID_PARAM (-3)
#define HDF_ERR_INVALID_OBJECT (-4)
#define HDF_ERR_MALLOC_FAIL (-6)
#define HDF_ERR_TIMEOUT     (-7)
#define HDF_ERR_IO          (-5)
#define HDF_ERR_DEVICE_BUSY (-8)
#define HDF_ERR_OUT_OF_RANGE (-9)
#define HDF_ERR_DEVICE_NODATA (-10)
typedef int32_t HDF_STATUS;

/* ---------- HDF I2C ---------- */
typedef void* DevHandle;

/* I2C message flags */
#define I2C_FLAG_READ       0x0001
#define I2C_FLAG_ADDR_10BIT 0x0010
#define I2C_FLAG_NO_START   0x4000
#define I2C_FLAG_IGNORE_NAK 0x1000
#define I2C_FLAG_NO_STOP    0x8000
#define I2C_FLAG_STOP       0x0000

struct I2cMsg {
    uint16_t addr;
    uint8_t *buf;
    uint16_t len;
    uint16_t flags;
};

DevHandle I2cOpen(int16_t number);
void I2cClose(DevHandle handle);
int32_t I2cTransfer(DevHandle handle, struct I2cMsg *msgs, int16_t count);

/* ---------- HDF GPIO ---------- */
#define GPIO_DIR_IN     0
#define GPIO_DIR_OUT    1
#define GPIO_VAL_LOW    0
#define GPIO_VAL_HIGH   1

typedef enum {
    GPIO_IRQ_TRIGGER_NONE       = 0,
    GPIO_IRQ_TRIGGER_RISING     = 1,
    GPIO_IRQ_TRIGGER_FALLING    = 2,
    GPIO_IRQ_TRIGGER_HIGH       = 4,
    GPIO_IRQ_TRIGGER_LOW        = 8,
} GpioIrqTrigger;

typedef int32_t (*GpioIrqFunc)(uint16_t gpio, void *data);

int32_t GpioSetDir(uint16_t gpio, uint16_t dir);
int32_t GpioGetDir(uint16_t gpio, uint16_t *dir);
int32_t GpioRead(uint16_t gpio, uint16_t *val);
int32_t GpioWrite(uint16_t gpio, uint16_t val);
int32_t GpioSetIrq(uint16_t gpio, uint16_t mode, GpioIrqFunc func, void *arg);
int32_t GpioUnsetIrq(uint16_t gpio, void *arg);
int32_t GpioEnableIrq(uint16_t gpio);
int32_t GpioDisableIrq(uint16_t gpio);

/* ---------- OSAL timing ---------- */
void OsalMSleep(uint32_t ms);
void OsalUSleep(uint32_t us);
void OsalSleep(uint32_t sec);
void OsalUDelay(uint32_t us);
void OsalMDelay(uint32_t ms);
uint64_t OsalGetSysTimeMs(void);

/* OSAL time struct */
typedef struct {
    uint32_t sec;
    uint32_t usec;
} OsalTimespec;

int32_t OsalGetTime(OsalTimespec *time);

/* ---------- OSAL mutex ---------- */
typedef struct { uint32_t dummy; } OsalMutex;

int32_t OsalMutexInit(OsalMutex *mutex);
int32_t OsalMutexDestroy(OsalMutex *mutex);
int32_t OsalMutexLock(OsalMutex *mutex);
int32_t OsalMutexTimedLock(OsalMutex *mutex, uint32_t ms);
int32_t OsalMutexUnlock(OsalMutex *mutex);

/* ---------- OSAL spinlock ---------- */
typedef struct { uint32_t dummy; } OsalSpinlock;
int32_t OsalSpinInit(OsalSpinlock *lock);
int32_t OsalSpinDestroy(OsalSpinlock *lock);
int32_t OsalSpinLock(OsalSpinlock *lock);
int32_t OsalSpinUnlock(OsalSpinlock *lock);

/* ---------- OSAL semaphore ---------- */
typedef struct { uint32_t dummy; } OsalSem;
int32_t OsalSemInit(OsalSem *sem, uint32_t value);
int32_t OsalSemWait(OsalSem *sem, uint32_t ms);
int32_t OsalSemPost(OsalSem *sem);
int32_t OsalSemDestroy(OsalSem *sem);

/* ---------- OSAL thread ---------- */
typedef struct { uint32_t dummy; } OsalThread;
typedef struct {
    const char *name;
    size_t stackSize;
    int32_t priority;
} OsalThreadParam;

int32_t OsalThreadCreate(OsalThread *thread, int (*threadEntry)(void *),
                          void *entryPara);
int32_t OsalThreadStart(OsalThread *thread, const OsalThreadParam *param);
int32_t OsalThreadDestroy(OsalThread *thread);

/* ---------- OSAL memory ---------- */
void *OsalMemAlloc(size_t size);
void *OsalMemCalloc(size_t size);
void OsalMemFree(void *mem);

/* ---------- HDF log ---------- */
#define HDF_LOGE(fmt, ...) ((void)0)
#define HDF_LOGW(fmt, ...) ((void)0)
#define HDF_LOGI(fmt, ...) ((void)0)
#define HDF_LOGD(fmt, ...) ((void)0)
#define HDF_LOG_TAG(tag)

/* ---------- HDF device model (minimal) ---------- */
struct HdfDeviceObject {
    void *service;
    void *property;
};

struct HdfDeviceIoClient {
    struct HdfDeviceObject *device;
    void *priv;
};

/* ---------- Misc ---------- */
int printf(const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __OPENHARMONY_LITEOSM_STUB_H__ */
