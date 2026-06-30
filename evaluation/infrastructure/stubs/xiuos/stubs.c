/* XiUOS stub implementations */
#include "xiuos.h"
#include <stdlib.h>

/* Transform layer file-descriptor stubs */
int PrivOpen(const char *path, int oflag) { (void)path;(void)oflag; return 3; }
int PrivClose(int fd) { (void)fd; return 0; }
int PrivRead(int fd, void *buf, size_t n) {
    (void)fd; if(buf && n) memset(buf,0x19,n); return (int)n;
}
int PrivWrite(int fd, const void *buf, size_t n) { (void)fd;(void)buf; return (int)n; }
int PrivIoctl(int fd, int cmd, void *arg) { (void)fd;(void)cmd;(void)arg; return 0; }
void PrivTaskDelay(int32_t ms) { (void)ms; }

/* Mutex */
int PrivMutexCreate(void **m, int a) { (void)m;(void)a; return 0; }
int PrivMutexDelete(void *m) { (void)m; return 0; }
int PrivMutexObtain(void *m) { (void)m; return 0; }
int PrivMutexAbandon(void *m) { (void)m; return 0; }

/* Semaphore */
int PrivSemaphoreCreate(void **s, int a, int c) { (void)s;(void)a;(void)c; return 0; }
int PrivSemaphoreDelete(void *s) { (void)s; return 0; }
int PrivSemaphoreObtainWait(void *s, int32_t ms) { (void)s;(void)ms; return 0; }
int PrivSemaphoreAbandon(void *s) { (void)s; return 0; }

/* Bus framework */
BusType BusFind(const char *name) { (void)name; return (BusType)1; }
HardwareDevType BusFindDevice(BusType bus, const char *name) {
    (void)bus;(void)name; return (HardwareDevType)1;
}
int BusDevOpen(HardwareDevType dev) { (void)dev; return 0; }
int BusDevClose(HardwareDevType dev) { (void)dev; return 0; }
int BusDevWriteData(HardwareDevType dev, struct BusBlockWriteParam *wp) { (void)dev;(void)wp; return 0; }
int BusDevReadData(HardwareDevType dev, struct BusBlockReadParam *rp) {
    (void)dev;(void)rp; return 0;
}
int BusDrvConfigure(HardwareDevType dev, void *cfg) { (void)dev;(void)cfg; return 0; }

/* I2C specific */
HardwareDevType I2cDeviceFind(const char *name, enum DevType dev_type) { (void)name; (void)dev_type; return (HardwareDevType)1; }
int I2cDeviceRegister(HardwareDevType dev, void *drv, const char *name) {
    (void)dev;(void)drv;(void)name; return 0;
}
int I2cDriverAttachToBus(const char *drv, const char *bus) { (void)drv;(void)bus; return 0; }
int I2cDeviceAttachToBus(const char *dev, const char *bus) { (void)dev;(void)bus; return 0; }

/* Memory */
void *PrivMalloc(size_t sz) { return malloc(sz); }
void *PrivCalloc(size_t n, size_t sz) { return calloc(n, sz); }
void PrivFree(void *p) { free(p); }

__attribute__((weak)) int main(void) { return 0; }
