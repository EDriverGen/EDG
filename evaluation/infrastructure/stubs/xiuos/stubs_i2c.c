/* Functional XiUOS I2C stubs — BusDevWriteData/BusDevReadData route through hw_i2c.h
 *
 * XiUOS uses its bus framework for I2C. The driver opens a bus, then calls
 * BusDevWriteData/BusDevReadData with BusBlockWriteParam/BusBlockReadParam
 * or uses PrivIoctl. For simplicity, PrivWrite/PrivRead also route through
 * hw_i2c since some drivers use the transform layer directly.
 */
#include "xiuos.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* Per-fd I2C slave address table.
 *
 * XiUOS drivers typically open a device path and then set the slave address
 * via PrivIoctl(OPE_INT, &PrivIoctlCfg{I2C_TYPE, &addr}) before
 * PrivRead/PrivWrite. A single-address scheme breaks devices that expose
 * multiple logical endpoints on the same bus (e.g. LSM303DLHC = accel@0x19
 * + mag@0x1E, where the driver opens the same "/dev/i2cN" twice and keeps
 * two distinct fds). We therefore track the slave address per fd so each
 * PrivRead/PrivWrite uses the address that was last set on that fd.
 *
 * The single-address symbol `_cur_i2c_addr` is kept as a fallback
 * and is updated on every ioctl so that code paths which ignore the fd
 * (e.g. BusDrvConfigure+BusDevWriteData convenience wrappers) continue to
 * work for single-address devices.
 */
#define XIUOS_MAX_FDS 8
static uint8_t _cur_i2c_addr = 0;
static uint8_t _fd_i2c_addr[XIUOS_MAX_FDS] = {0};
static int _next_fd = 3;
static uint8_t _bus_i2c_addr = 0x23;

static int _fd_index(int fd) {
    int idx = fd - 3;
    if (idx < 0 || idx >= XIUOS_MAX_FDS) return -1;
    return idx;
}

static uint8_t _fd_get_addr(int fd) {
    int idx = _fd_index(fd);
    if (idx < 0) return _cur_i2c_addr;
    uint8_t a = _fd_i2c_addr[idx];
    return a ? a : _cur_i2c_addr;
}

static void _fd_set_addr(int fd, uint8_t addr) {
    int idx = _fd_index(fd);
    if (idx >= 0) _fd_i2c_addr[idx] = addr;
    _cur_i2c_addr = addr;
}

/* ── Transform layer stubs (with I2C routing) ─────────── */

int PrivOpen(const char *path, int oflag) {
    (void)path; (void)oflag;
    if (_next_fd >= 3 + XIUOS_MAX_FDS) _next_fd = 3;
    int fd = _next_fd++;
    int idx = _fd_index(fd);
    if (idx >= 0) _fd_i2c_addr[idx] = 0;
    return fd;
}
int PrivClose(int fd) { (void)fd; return 0; }

int PrivRead(int fd, void *buf, size_t n) {
    uint8_t addr = _fd_get_addr(fd);
    return hw_i2c_read(0, addr, buf, (uint16_t)n) == 0 ? (int)n : -1;
}

int PrivWrite(int fd, const void *buf, size_t n) {
    uint8_t addr = _fd_get_addr(fd);
    return hw_i2c_write(0, addr, buf, (uint16_t)n) == 0 ? (int)n : -1;
}

int PrivIoctl(int fd, int cmd, void *arg) {
    /* Case 1: direct I2C_TYPE call (BusDevIoctl style) */
    if (cmd == I2C_TYPE && arg) {
        I2cDataStandard *msg = (I2cDataStandard *)arg;
        _fd_set_addr(fd, (uint8_t)msg->addr);
        _bus_i2c_addr = (uint8_t)msg->addr;
        if (msg->flags & I2C_M_RD) {
            return hw_i2c_read(0, (uint8_t)msg->addr, msg->buf, (uint16_t)msg->len);
        } else {
            return hw_i2c_write(0, (uint8_t)msg->addr, msg->buf, (uint16_t)msg->len);
        }
    }
    /* Case 2: PrivIoctl(fd, OPE_INT, &PrivIoctlCfg) — transform-layer pattern.
     * The driver uses PrivIoctlCfg { .ioctl_driver_type = I2C_TYPE, .args = &addr }
     * to set the slave address before PrivRead/PrivWrite. */
    if (cmd == 0 /* OPE_INT */ && arg) {
        typedef struct { int ioctl_driver_type; void *args; } _IoctlCfg;
        _IoctlCfg *cfg = (_IoctlCfg *)arg;
        if (cfg->ioctl_driver_type == I2C_TYPE && cfg->args) {
            _fd_set_addr(fd, (uint8_t)(*(uint16_t *)cfg->args));
            return 0;
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

/* ── Bus framework (with I2C routing) ─────────────────── */
BusType BusFind(const char *name) { (void)name; return (BusType)1; }
HardwareDevType BusFindDevice(BusType bus, const char *name) { (void)bus;(void)name; return (HardwareDevType)1; }
int BusDevOpen(HardwareDevType dev) { (void)dev; hw_i2c1_init(); return 0; }
int BusDevClose(HardwareDevType dev) { (void)dev; return 0; }

int BusDevWriteData(HardwareDevType dev, struct BusBlockWriteParam *wp) {
    (void)dev;
    x_size_t len = wp ? (wp->size ? wp->size : wp->length) : 0;
    uint8_t addr = wp && wp->address ? (uint8_t)wp->address : _bus_i2c_addr;
    if (!wp || !wp->buffer || len == 0) return -1;
    return hw_i2c_write(0, addr, (const uint8_t *)wp->buffer, (uint16_t)len) == 0 ? 0 : -1;
}

int BusDevReadData(HardwareDevType dev, struct BusBlockReadParam *rp) {
    (void)dev;
    x_size_t len = rp ? (rp->size ? rp->size : rp->length) : 0;
    uint8_t addr = rp && rp->address ? (uint8_t)rp->address : _bus_i2c_addr;
    if (!rp || !rp->buffer || len == 0) return -1;
    int rc = hw_i2c_read(0, addr, (uint8_t *)rp->buffer, (uint16_t)len);
    if (rc == 0) {
        rp->read_length = len;
    }
    return rc == 0 ? 0 : -1;
}

int BusDrvConfigure(HardwareDevType dev, void *cfg) { (void)dev;(void)cfg; return 0; }
int DeviceObtainBus(BusType bus, HardwareDevType dev, const char *drv,
                    struct BusConfigureInfo *cfg) {
    (void)bus;
    (void)dev;
    (void)drv;
    (void)cfg;
    hw_i2c1_init();
    return 0;
}

/* ── I2C specific ─────────────────────────────────────── */
HardwareDevType I2cDeviceFind(const char *name, enum DevType dev_type) {
    (void)name; (void)dev_type;
    _bus_i2c_addr = 0x23;
    return (HardwareDevType)1;
}
int I2cDeviceRegister(HardwareDevType dev, void *drv, const char *name) { (void)dev;(void)drv;(void)name; return 0; }
int I2cDriverAttachToBus(const char *drv, const char *bus) { (void)drv;(void)bus; return 0; }
int I2cDeviceAttachToBus(const char *dev, const char *bus) { (void)dev;(void)bus; return 0; }

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
