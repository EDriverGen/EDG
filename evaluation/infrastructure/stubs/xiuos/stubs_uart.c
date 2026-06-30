/* Functional XiUOS UART stubs — PrivWrite/PrivRead route through hw_uart_bus.h */
#include "xiuos.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ── Transform layer stubs (with UART routing) ────────── */

int PrivOpen(const char *path, int oflag) {
    (void)path; (void)oflag;
    hw_uart_bus_init();
    return 3;
}

int PrivClose(int fd) { (void)fd; return 0; }

int PrivWrite(int fd, const void *buf, size_t n) {
    (void)fd;
    const uint8_t *p = (const uint8_t *)buf;
    for (size_t i = 0; i < n; i++)
        hw_uart_bus_write_byte(p[i]);
    return (int)n;
}

int PrivRead(int fd, void *buf, size_t n) {
    (void)fd;
    uint8_t *p = (uint8_t *)buf;
    for (size_t i = 0; i < n; i++) {
        if (hw_uart_bus_read_byte(&p[i]) != 0)
            return (int)i;
    }
    return (int)n;
}

int PrivIoctl(int fd, int cmd, void *arg) { (void)fd; (void)cmd; (void)arg; return 0; }

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
int BusDevOpen(HardwareDevType dev) { (void)dev; hw_uart_bus_init(); return 0; }
int BusDevClose(HardwareDevType dev) { (void)dev; return 0; }
int BusDevWriteData(HardwareDevType dev, void *wp) { (void)dev;(void)wp; return 0; }
int BusDevReadData(HardwareDevType dev, void *rp) { (void)dev;(void)rp; return 0; }
int BusDrvConfigure(HardwareDevType dev, void *cfg) { (void)dev;(void)cfg; return 0; }

/* ── Memory ───────────────────────────────────────────── */
void *PrivMalloc(size_t sz) { return malloc(sz); }
void *PrivCalloc(size_t n, size_t sz) { return calloc(n, sz); }
void PrivFree(void *p) { free(p); }

/* ── printf via UART2 ─────────────────────────────────── */
int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
