/* Functional OpenHarmony LiteOS-M GPIO stubs — GpioWrite/GpioRead drive STM32 GPIO MMIO.
 *
 * HDF GPIO API uses a flat pin number (uint16_t gpio).
 * Mapping: gpio = port*16 + pin_num (OpenHarmony convention).
 * Port 0→GPIOA, 1→GPIOB, 2→GPIOC, 3→GPIOD, 4→GPIOE.
 */
#include "openharmony_liteosm.h"
#include "gpio_if.h"
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

static const uint32_t _port_base[5] = {
    GPIOA_BASE_, GPIOB_BASE_, GPIOC_BASE_, GPIOD_BASE_, GPIOE_BASE_
};

static inline uint32_t _gpio_base(uint16_t gpio) {
    unsigned port = gpio / 16;
    return (port < 5) ? _port_base[port] : GPIOB_BASE_;
}

static inline uint32_t _gpio_pin(uint16_t gpio) {
    return gpio % 16;
}

/* ── GPIO stubs (real MMIO) ───────────────────────────── */

int32_t GpioSetDir(uint16_t gpio, uint16_t dir) {
    unsigned port = gpio / 16;
    unsigned pin = gpio % 16;
    if (port > 4 || pin > 15) return HDF_ERR_INVALID_PARAM;

    /* Enable clock: IOPAEN=bit2, ..., IOPEEN=bit6 */
    RCC_APB2ENR |= (1U << (port + 2));

    uint32_t base = _port_base[port];
    int shift = (pin < 8 ? pin : pin - 8) * 4;
    volatile uint32_t *reg = (pin < 8)
        ? (volatile uint32_t*)(base + 0x00)
        : (volatile uint32_t*)(base + 0x04);

    uint32_t v = *reg;
    v &= ~(0xFU << shift);
    if (dir == GPIO_DIR_OUT)
        v |= (0x1U << shift); /* push-pull 10 MHz */
    else
        v |= (0x4U << shift); /* floating input */
    *reg = v;
    return HDF_SUCCESS;
}

int32_t GpioGetDir(uint16_t gpio, uint16_t *dir) {
    (void)gpio;
    if (dir) *dir = GPIO_DIR_IN;
    return HDF_SUCCESS;
}

int32_t GpioRead(uint16_t gpio, uint16_t *val) {
    if (!val) return HDF_ERR_INVALID_PARAM;
    *val = (GPIO_IDR(_gpio_base(gpio)) >> _gpio_pin(gpio)) & 1U;
    return HDF_SUCCESS;
}

int32_t GpioWrite(uint16_t gpio, uint16_t val) {
    uint32_t base = _gpio_base(gpio);
    uint32_t pin = _gpio_pin(gpio);
    if (val)
        GPIO_BSRR(base) = (1U << pin);
    else
        GPIO_BSRR(base) = (1U << (pin + 16));
    return HDF_SUCCESS;
}

int32_t GpioSetIrq(uint16_t gpio, uint16_t mode, GpioIrqFunc func, void *arg) {
    (void)gpio;(void)mode;(void)func;(void)arg; return HDF_SUCCESS;
}
int32_t GpioUnsetIrq(uint16_t gpio, void *arg) { (void)gpio;(void)arg; return HDF_SUCCESS; }
int32_t GpioEnableIrq(uint16_t gpio) { (void)gpio; return HDF_SUCCESS; }
int32_t GpioDisableIrq(uint16_t gpio) { (void)gpio; return HDF_SUCCESS; }

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

/* ── UART dummies ─────────────────────────────────────── */
DevHandle UartOpen(uint32_t port) { (void)port; return (DevHandle)1; }
void UartClose(DevHandle handle) { (void)handle; }
int32_t UartRead(DevHandle handle, uint8_t *data, uint32_t size) { (void)handle; if(data&&size) memset(data,0,size); return (int32_t)size; }
int32_t UartWrite(DevHandle handle, uint8_t *data, uint32_t size) { (void)handle;(void)data; return (int32_t)size; }
int32_t UartSetBaud(DevHandle handle, uint32_t baud) { (void)handle;(void)baud; return HDF_SUCCESS; }
int32_t UartGetBaud(DevHandle handle, uint32_t *baud) { (void)handle; if(baud) *baud=9600; return HDF_SUCCESS; }
int32_t UartSetAttribute(DevHandle handle, struct UartAttribute *a) { (void)handle;(void)a; return HDF_SUCCESS; }
int32_t UartGetAttribute(DevHandle handle, struct UartAttribute *a) { (void)handle;(void)a; return HDF_SUCCESS; }
int32_t UartSetTransMode(DevHandle handle, uint32_t mode) { (void)handle;(void)mode; return HDF_SUCCESS; }

/* ── OSAL ─────────────────────────────────────────────── */
void OsalMSleep(uint32_t ms) { (void)ms; }
void OsalSleep(uint32_t sec) { (void)sec; }
void OsalUDelay(uint32_t us) { (void)us; }
void OsalMDelay(uint32_t ms) { (void)ms; }
uint64_t OsalGetSysTimeMs(void) { static uint64_t ms = 0; return ms++; }
int32_t OsalGetTime(OsalTimespec *t) {
    static uint64_t us_counter = 0;
    us_counter += 1;
    if (t) { t->sec = (uint32_t)(us_counter / 1000000ULL); t->usec = (uint32_t)(us_counter % 1000000ULL); }
    return HDF_SUCCESS;
}

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
