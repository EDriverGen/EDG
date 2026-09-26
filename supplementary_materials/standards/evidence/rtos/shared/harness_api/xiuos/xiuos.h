/*
 * XiUOS unified stub for syntax-only compilation tests.
 * Covers: XiZi transform layer (PrivOpen/Read/Write/Ioctl/Close, PrivTaskDelay),
 *         I2C bus/device model, GPIO bus/pin, bus framework.
 */
#ifndef __XIUOS_STUB_H__
#define __XIUOS_STUB_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- XiUOS base types ---------- */
typedef int x_err_t;
typedef unsigned int x_size_t;
typedef int x_base_t;
typedef unsigned int x_ubase_t;
typedef long long x_ticks_t;
typedef int x_OffPos;

#ifndef NULL
#define NULL ((void*)0)
#endif
#define EOK          0
#define ERROR       (-1)
#define ENOMEMORY   (-2)
#define EEMPTY      (-3)
#define EFULL       (-4)
#define ETIMEOUT    (-5)
#define EBUSY       (-6)
#define EINVALED    (-7)
#define ENOSYS      (-8)

/* ---------- File descriptor style (transform layer) ---------- */
int PrivOpen(const char *path, int oflag);
int PrivClose(int fd);
int PrivRead(int fd, void *buf, size_t nbytes);
int PrivWrite(int fd, const void *buf, size_t nbytes);
int PrivIoctl(int fd, int cmd, void *arg);
void PrivTaskDelay(int32_t ms);
int PrivMutexCreate(void **mutex, int attr);
int PrivMutexDelete(void *mutex);
int PrivMutexObtain(void *mutex);
int PrivMutexAbandon(void *mutex);
int PrivSemaphoreCreate(void **sem, int attr, int init_count);
int PrivSemaphoreDelete(void *sem);
int PrivSemaphoreObtainWait(void *sem, int32_t ms);
int PrivSemaphoreAbandon(void *sem);

/* Open flags */
#define OPE_INT      0x00000000
#define OPE_POLL     0x00000001
#define O_RDONLY     0x0000
#define O_WRONLY     0x0001
#define O_RDWR       0x0002

/* ---------- XiUOS types for I2C ---------- */
#define I2C_TYPE              0x0001

enum DevType {
    TYPE_I2C_DEV = 0,
    TYPE_DEV_END,
};

typedef struct BusConfigureInfo {
    int configure_cmd;
    void *private_data;
} BusConfigureInfo;

typedef struct BusBlockReadParam {
    x_OffPos pos;
    x_OffPos start_address;
    void *buffer;
    x_size_t size;
    x_size_t length;
    x_size_t read_length;
    uint16_t address;
    uint32_t timeout;
} BusBlockReadParam;

typedef struct BusBlockWriteParam {
    x_OffPos pos;
    x_OffPos start_address;
    const void *buffer;
    x_size_t size;
    x_size_t length;
    uint16_t address;
    uint32_t timeout;
} BusBlockWriteParam;

struct Bus;
struct HardwareDev {
    int dummy;
};
struct Driver;
#define I2cHardwareDevice HardwareDev

typedef struct {
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t *buf;
} I2cDataStandard;

/* I2C flags */
#define I2C_M_RD         0x0001
#define I2C_M_WR         0x0000
#define I2C_M_TEN        0x0010
#define I2C_M_NOSTART    0x4000
#define I2C_M_REV_DIR_ADDR 0x2000
#define I2C_M_IGNORE_NAK   0x1000
#define I2C_M_NO_RD_ACK    0x0800

/* ---------- XiUOS Bus framework ---------- */
typedef struct Bus *BusType;
typedef struct HardwareDev *HardwareDevType;
typedef struct Driver *DriverType;

/* Bus find & device lookup */
BusType BusFind(const char *bus_name);
HardwareDevType BusFindDevice(BusType bus, const char *dev_name);
int BusDevOpen(HardwareDevType dev);
int BusDevClose(HardwareDevType dev);
int BusDevWriteData(HardwareDevType dev, struct BusBlockWriteParam *write_param);
int BusDevReadData(HardwareDevType dev, struct BusBlockReadParam *read_param);
int BusDrvConfigure(HardwareDevType dev, void *cfg);
int DeviceObtainBus(BusType bus, HardwareDevType dev, const char *drv_name,
                    struct BusConfigureInfo *configure_info);

/* I2C specific */
HardwareDevType I2cDeviceFind(const char *dev_name, enum DevType dev_type);
int I2cDeviceRegister(HardwareDevType dev, void *drv, const char *dev_name);
int I2cDriverAttachToBus(const char *drv_name, const char *bus_name);
int I2cDeviceAttachToBus(const char *dev_name, const char *bus_name);

/* ---------- GPIO ---------- */
#define GPIO_CONFIG_MODE  0x0001
#define GPIO_CONFIG_PULL  0x0002
#define GPIO_CFG_OUTPUT     0x01
#define GPIO_CFG_OUTPUT_OD  0x02
#define GPIO_CFG_INPUT      0x03
#define GPIO_CFG_INPUT_PULLUP   0x04
#define GPIO_CFG_INPUT_PULLDOWN 0x05

typedef struct {
    uint16_t cmd;
    uint16_t pin;
    uint16_t mode;
    uint16_t pull;
} GpioConfigParam;

typedef struct {
    uint16_t cmd;
    void *args;
} gpio_param;

/* XiUOS GPIO config param (used by PrivIoctl for pin configuration) */
struct PinParam {
    uint16_t cmd;
    uint16_t mode;
    uint16_t pin;
};

/* ---------- SPI ---------- */
#define SPI_IOC_TRANSFER  0x6B01

struct spi_ioc_transfer {
    const void *tx_buf;
    void *rx_buf;
    uint32_t len;
    uint32_t speed_hz;
    uint16_t delay_usecs;
    uint8_t bits_per_word;
    uint8_t cs_change;
};

/* XiUOS SPI data param (used by PrivIoctl) */
typedef struct SpiDataParam {
    const void *tx_buff;
    void *rx_buff;
    uint32_t length;
} SpiDataParam;

/* ---------- PrivIoctl config ---------- */
typedef struct PrivIoctlCfg {
    int ioctl_driver_type;
    void *args;
} PrivIoctlCfg;

/* ---------- Misc ---------- */
void *PrivMalloc(size_t size);
void *PrivCalloc(size_t num, size_t size);
void PrivFree(void *ptr);
int printf(const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);

/* POSIX shims needed by generated GPIO drivers */
static inline int open(const char *path, int flags, ...) {
    (void)flags; return PrivOpen(path, 0);
}
static inline int close(int fd) { return PrivClose(fd); }
static inline int read(int fd, void *buf, size_t nbytes) {
    return PrivRead(fd, buf, nbytes);
}
static inline int write(int fd, const void *buf, size_t nbytes) {
    return PrivWrite(fd, buf, nbytes);
}
static inline int ioctl(int fd, int cmd, ...) {
    va_list ap;
    void *arg = NULL;
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);
    return PrivIoctl(fd, cmd, arg);
}
static inline int usleep(unsigned int usec) { PrivTaskDelay((usec + 999) / 1000); return 0; }

#ifdef __cplusplus
}
#endif

#endif /* __XIUOS_STUB_H__ */
