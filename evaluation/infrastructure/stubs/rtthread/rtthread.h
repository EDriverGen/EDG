#ifndef __RTTHREAD_H__
#define __RTTHREAD_H__
/*
 * RT-Thread API stub for cross-compilation testing.
 * Provides type definitions and function declarations matching RT-Thread's
 * public API surface.  Not a real kernel — just enough for GCC to verify
 * that driver code uses correct types and signatures.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Core types ---- */
typedef int32_t   rt_err_t;
typedef int32_t   rt_base_t;
typedef uint32_t  rt_uint32_t;
typedef uint16_t  rt_uint16_t;
typedef uint8_t   rt_uint8_t;
typedef int32_t   rt_int32_t;
typedef int16_t   rt_int16_t;
typedef int8_t    rt_int8_t;
typedef uint64_t  rt_uint64_t;
typedef int64_t   rt_int64_t;
typedef rt_base_t rt_bool_t;
typedef rt_uint32_t rt_size_t;
typedef int32_t   rt_ssize_t;
typedef rt_uint32_t rt_tick_t;
typedef rt_base_t rt_off_t;

/* ---- Constants ----
 * Negative-integer error codes kept from original stub for backward
 * compatibility with earlier generated drivers (they often do
 * `return -RT_EINVAL;`).  Upstream RT-Thread's klibc/kerrno.h uses
 * positive values (matching POSIX errno), but we preserve the existing
 * signs here because the stub's sole job is to compile, and existing
 * baselines rely on these exact values.  Newly added symbols below use
 * values that are consistent with the existing negative scheme and
 * unique enough not to collide.
 */
#define RT_NULL     NULL
#define RT_EOK      0
#define RT_ERROR    (-1)
#define RT_EINVAL   (-22)
#define RT_EIO      (-5)
#define RT_ETIMEOUT (-116)
#define RT_ENOSYS   (-38)
#define RT_ENOMEM   (-12)
#define RT_EBUSY    (-16)
#define RT_ENODEV   (-19)
#define RT_ENOSPC   (-28)
#define RT_EFULL    (-28)
/* --- Added to match upstream klibc/kerrno.h ---
 * These are required by drivers that propagate detailed error codes
 * (e.g. `return -RT_EPERM;` when hardware isn't ready, or
 * `return -RT_ENOENT;` when probing a missing device).  Values chosen
 * to mirror Linux errno negatives so existing negative-return idioms
 * continue to work with `strerror`-style lookups in test code.
 */
#define RT_EPERM        (-1)    /* Operation not permitted */
#define RT_ENOENT       (-2)    /* No such entry */
#define RT_EINTR        (-4)    /* Interrupted system call */
#define RT_EFAULT       (-14)   /* Bad address */
#define RT_ENOBUFS      (-105)  /* No buffer space available */
#define RT_EAGAIN       (-11)   /* Try again / would block */
#define RT_ETRAP        (-254) /* Trap event */
#define RT_ESCHEDISR    (-253) /* Scheduler failure in ISR context */
#define RT_ESCHEDLOCKED (-252) /* Scheduler failure in critical region */
#define RT_EEMPTY       (-61)  /* Resource empty (mirrors ENODATA) */

/* --- POSIX errno aliases ---
 * Real RT-Thread ships a klibc shim that, when C library support is
 * enabled, maps the familiar POSIX names (`EIO`, `EINVAL`, `EPERM`, …)
 * onto the RT-Thread signed-negative variants.  Hand-written and
 * generated drivers freely mix the two spellings because RT-Thread
 * samples themselves do.  Provide the mapping here so
 *
 *     if (bus == NULL) return -EINVAL;
 *
 * compiles identically to
 *
 *     if (bus == NULL) return -RT_EINVAL;
 *
 * The aliases are defined only when the host <errno.h> hasn't already
 * defined them (e.g. when the driver explicitly `#include <errno.h>`
 * and the POSIX values are positive).  When the driver skips
 * `<errno.h>`, the bare macros resolve to our RT-Thread values —
 * which is the intent when running inside the kernel anyway.
 */
#ifndef EPERM
#define EPERM     1
#endif
#ifndef ENOENT
#define ENOENT    2
#endif
#ifndef EINTR
#define EINTR     4
#endif
#ifndef EIO
#define EIO       5
#endif
#ifndef ENXIO
#define ENXIO     6
#endif
#ifndef ENOEXEC
#define ENOEXEC   8
#endif
#ifndef EBADF
#define EBADF     9
#endif
#ifndef EAGAIN
#define EAGAIN    11
#endif
#ifndef ENOMEM
#define ENOMEM    12
#endif
#ifndef EFAULT
#define EFAULT    14
#endif
#ifndef EBUSY
#define EBUSY     16
#endif
#ifndef EEXIST
#define EEXIST    17
#endif
#ifndef ENODEV
#define ENODEV    19
#endif
#ifndef EINVAL
#define EINVAL    22
#endif
#ifndef ENOSPC
#define ENOSPC    28
#endif
#ifndef EPIPE
#define EPIPE     32
#endif
#ifndef ERANGE
#define ERANGE    34
#endif
#ifndef ENOSYS
#define ENOSYS    38
#endif
#ifndef ENODATA
#define ENODATA   61
#endif
#ifndef EPROTO
#define EPROTO    71
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT 110
#endif
#ifndef EINPROGRESS
#define EINPROGRESS 115
#endif

#define RT_FALSE    0
#define RT_TRUE     1
#ifndef RT_ASSERT
#define RT_ASSERT(EX) do { (void)(EX); } while (0)
#endif
#define RT_WAITING_FOREVER  (-1)
#define RT_THREAD_PRIORITY_MAX  32
#define RT_MAIN_THREAD_PRIORITY (RT_THREAD_PRIORITY_MAX / 3)
#define RT_MAIN_THREAD_STACK_SIZE  2048
#define RT_TIMER_FLAG_DEACTIVATED  0x00
#define RT_TIMER_FLAG_ACTIVATED    0x01
#define RT_TIMER_FLAG_ONE_SHOT     0x00
#define RT_TIMER_FLAG_PERIODIC     0x02
#define RT_IPC_FLAG_FIFO    0x00
#define RT_IPC_FLAG_PRIO    0x01

/* ---- RT-Thread object model ----
 * Real RT-Thread embeds an rt_object base in every bus/device/thread
 * struct.  Some generated drivers register themselves by setting
 * `bus->parent.type = RT_Object_Class_*` during init.  We provide the
 * enum + an rt_object shell so such code compiles; the runtime
 * behavior is a no-op because generated drivers do not rely on
 * the real object manager in the stub harness.
 */
enum rt_object_class_type {
    RT_Object_Class_Null = 0x00,
    RT_Object_Class_Thread,
    RT_Object_Class_Semaphore,
    RT_Object_Class_Mutex,
    RT_Object_Class_Event,
    RT_Object_Class_MailBox,
    RT_Object_Class_MessageQueue,
    RT_Object_Class_MemHeap,
    RT_Object_Class_MemPool,
    RT_Object_Class_Device,
    RT_Object_Class_Timer,
    RT_Object_Class_Module,
    RT_Object_Class_Memory,
    RT_Object_Class_Channel,
    RT_Object_Class_Custom,
    RT_Object_Class_Unknown,
    RT_Object_Class_Static
};

struct rt_object {
    char        name[8];
    rt_uint8_t  type;
    rt_uint8_t  flag;
    void       *list_prev;  /* simplified rt_list_t */
    void       *list_next;
};

/* ---- I2C ---- */
#define RT_I2C_WR        0x0000
#define RT_I2C_RD        0x0001
#define RT_I2C_NO_START  0x0010
#define RT_I2C_NO_STOP   0x0020

struct rt_i2c_bus_device {
    struct rt_object parent;  /* RT-Thread object base */
    void            *priv;    /* bus private data (unused in stub) */
};
typedef struct rt_i2c_bus_device rt_i2c_bus_device_t;

struct rt_i2c_msg {
    rt_uint16_t addr;
    rt_uint16_t flags;
    rt_uint16_t len;
    rt_uint8_t  *buf;
};

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name);
rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg msgs[],
                          rt_uint32_t num);
rt_size_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t addr, rt_uint16_t flags,
                             const rt_uint8_t *buf, rt_uint32_t count);
rt_size_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t addr, rt_uint16_t flags,
                             rt_uint8_t *buf, rt_uint32_t count);
rt_err_t rt_i2c_control(struct rt_i2c_bus_device *bus,
                        rt_uint32_t cmd,
                        void *arg);

/* Official RT-Thread I2C bus control helpers.
 * Upstream (components/drivers/include/drivers/dev_i2c.h, commit 2021-04-20
 * by RiceChen) declares these as `rt_inline` wrappers around
 * rt_mutex_take/release on the bus's internal lock. Drivers that want to
 * guarantee atomicity of a multi-message sequence (write-reg-then-read,
 * burst read across threads, etc.) are expected to use this pair.
 *
 * Our cross-compile stubs run in a single-thread bare-metal harness so
 * both calls are no-ops. Keeping them as ordinary functions (rather than
 * rt_inline) lets us avoid pulling struct rt_mutex into the bus device
 * definition, which would in turn break other drivers that memset the
 * bus struct. */
rt_err_t rt_i2c_bus_lock(struct rt_i2c_bus_device *bus, rt_tick_t timeout);
rt_err_t rt_i2c_bus_unlock(struct rt_i2c_bus_device *bus);


/* I2C control commands */
#define RT_I2C_DEV_CTRL_10BIT     0x20
#define RT_I2C_DEV_CTRL_TIMEOUT   0x21
#define RT_I2C_DEV_CTRL_RW        0x22
#define RT_I2C_DEV_CTRL_CLK       0x23
#define RT_I2C_DEV_CTRL_GET_ERROR 0x24
#define RT_I2C_CTRL_SET_MAX_HZ    RT_I2C_DEV_CTRL_CLK

/* ---- SPI ---- */
#define RT_SPI_MODE_0   (0 | 0)
#define RT_SPI_MODE_1   (0 | 1)
#define RT_SPI_MODE_2   (2 | 0)
#define RT_SPI_MODE_3   (2 | 1)
#define RT_SPI_MSB      0
#define RT_SPI_LSB      (1 << 2)
#define RT_SPI_MASTER   0
#define RT_SPI_SLAVE    (1 << 3)
#define RT_SPI_CS_HIGH  (1 << 4)
#define RT_SPI_NO_CS    (1 << 5)
#define RT_SPI_3WIRE    (1 << 6)

struct rt_spi_configuration {
    rt_uint8_t  mode;
    rt_uint8_t  data_width;
    rt_uint16_t reserved;
    rt_uint32_t max_hz;
};

struct rt_spi_device {
    struct rt_spi_configuration config;
    int dummy;
};

rt_err_t rt_spi_configure(struct rt_spi_device *device,
                          struct rt_spi_configuration *cfg);
rt_err_t rt_spi_send(struct rt_spi_device *device,
                     const void *send_buf, rt_size_t length);
rt_err_t rt_spi_recv(struct rt_spi_device *device,
                     void *recv_buf, rt_size_t length);
rt_err_t rt_spi_send_then_recv(struct rt_spi_device *device,
                               const void *send_buf, rt_size_t send_length,
                               void *recv_buf, rt_size_t recv_length);
rt_err_t rt_spi_transfer(struct rt_spi_device *device,
                         const void *send_buf, void *recv_buf,
                         rt_size_t length);
rt_err_t rt_spi_send_then_send(struct rt_spi_device *device,
                               const void *buf1, rt_size_t len1,
                               const void *buf2, rt_size_t len2);
struct rt_spi_device *rt_spi_bus_attach_device(struct rt_spi_device *device,
                                               const char *name,
                                               const char *bus_name,
                                               void *user_data);

/* ---- Serial / UART ---- */
#define RT_DEVICE_CTRL_CONFIG   0x03
#define RT_DEVICE_CTRL_SET_INT  0x10
#define RT_DEVICE_CTRL_CLR_INT  0x11
#define RT_DEVICE_FLAG_DMA_RX   0x200
#define RT_DEVICE_FLAG_DMA_TX   0x400
#define RT_DEVICE_FLAG_RDONLY    0x001
#define RT_DEVICE_FLAG_WRONLY    0x002
#define RT_SERIAL_EVENT_RX_IND  0x01

#define BAUD_RATE_2400   2400
#define BAUD_RATE_4800   4800
#define BAUD_RATE_9600   9600
#define BAUD_RATE_19200  19200
#define BAUD_RATE_38400  38400
#define BAUD_RATE_57600  57600
#define BAUD_RATE_115200 115200
#define DATA_BITS_8      8
#define STOP_BITS_1      0
#define PARITY_NONE      0

struct serial_configure {
    rt_uint32_t baud_rate;
    rt_uint32_t data_bits;
    rt_uint32_t stop_bits;
    rt_uint32_t parity;
    rt_uint32_t bit_order;
    rt_uint32_t invert;
    rt_uint32_t bufsz;
    rt_uint32_t flowcontrol;
};

#define RT_SERIAL_CONFIG_DEFAULT           \
{                                          \
    BAUD_RATE_115200,                      \
    DATA_BITS_8,                           \
    STOP_BITS_1,                           \
    PARITY_NONE,                           \
    0,                                     \
    0,                                     \
    64,                                    \
    0,                                     \
}

/* ---- Interrupt control ---- */
rt_base_t rt_hw_interrupt_disable(void);
void rt_hw_interrupt_enable(rt_base_t level);

/* ---- Logging / printf ----
 * Real RT-Thread `include/rtthread.h` does NOT define LOG_*; those macros
 * live exclusively in `rtdbg.h`.  The stub provides fallback definitions
 * so drivers that only include rtthread.h (and never rtdbg.h) still
 * compile, but each fallback is `#ifndef`-guarded so a prior or later
 * `#include "rtdbg.h"` wins without `-Wmacro-redefined` noise.
 */
#define rt_kprintf  printf
#ifndef LOG_E
#define LOG_E(...)  printf(__VA_ARGS__)
#endif
#ifndef LOG_W
#define LOG_W(...)  printf(__VA_ARGS__)
#endif
#ifndef LOG_I
#define LOG_I(...)  printf(__VA_ARGS__)
#endif
#ifndef LOG_D
#define LOG_D(...)  printf(__VA_ARGS__)
#endif

/* ---- Delay ----
 * Real RT-Thread (`src/thread.c:784`, `include/rtthread.h:162`):
 *     rt_err_t rt_thread_mdelay(rt_int32_t ms);
 * Drivers routinely write
 *     if (rt_thread_mdelay(ms) != RT_EOK) { ... }
 * so the stub MUST preserve that return type or the comparison becomes
 * a C "void value not ignored" hard error.  The parameter type is
 * signed per the upstream header; callers compile equally with
 * ``rt_int32_t``, ``rt_uint32_t`` or plain int arguments.
 */
rt_err_t rt_thread_mdelay(rt_int32_t ms);
#define rt_hw_us_delay(us) ((void)(us))
rt_uint32_t rt_tick_from_millisecond(rt_uint32_t ms);
rt_tick_t rt_tick_get(void);
/* rt_thread_delay / rt_thread_delay_until — take ticks, alias of mdelay
 * for stub purposes.  Real RT-Thread converts tick-count to ms via
 * RT_TICK_PER_SECOND; the stub's sole obligation is to compile
 * unchanged so device drivers that call these instead of mdelay
 * don't trip implicit-declaration errors. */
rt_err_t rt_thread_delay(rt_tick_t tick);
rt_err_t rt_thread_delay_until(rt_tick_t *tick, rt_tick_t inc_tick);
/* rt_tick_get_delta(since) — returns ticks elapsed since ``since``.
 * Real RT-Thread uses this for polling-style timeout loops; the stub
 * returns 0 so any ``while (rt_tick_get_delta(t0) < TIMEOUT)`` exits
 * immediately, which keeps init routines quick during compile-only
 * tests.  The sign is intentionally ``rt_tick_t`` to match upstream. */
rt_tick_t rt_tick_get_delta(rt_tick_t since);

/* ---- Memory ---- */
#define rt_memcpy   memcpy
#define rt_memset   memset
#define rt_malloc   malloc
#define rt_free     free
#define rt_calloc   calloc
void *rt_realloc(void *ptr, rt_size_t size);

/* ---- Device ---- */
typedef void *rt_device_t;
#define rt_device_find(name) ((rt_device_t)rt_i2c_bus_device_find(name))
rt_err_t rt_device_open(rt_device_t dev, rt_uint16_t oflag);
rt_err_t rt_device_close(rt_device_t dev);
rt_err_t rt_device_control(rt_device_t dev, int cmd, void *arg);
rt_size_t rt_device_read(rt_device_t dev, rt_off_t pos, void *buf, rt_size_t size);
rt_size_t rt_device_write(rt_device_t dev, rt_off_t pos, const void *buf, rt_size_t size);
#define RT_DEVICE_OFLAG_RDWR  0x003
#define RT_DEVICE_FLAG_INT_RX 0x100

/* ---- MSH ---- */
#define MSH_CMD_EXPORT(cmd, desc)
#define INIT_APP_EXPORT(fn)
#define INIT_COMPONENT_EXPORT(fn)

/* ---- GPIO / PIN ---- */
#define GET_PIN(port, pin)  ((port) * 16 + (pin))
#define A 0
#define B 1
#define C 2
#define D 3
#define PIN_MODE_OUTPUT  1
#define PIN_MODE_INPUT   0
#define PIN_LOW   0
#define PIN_HIGH  1

void rt_pin_mode(rt_base_t pin, rt_base_t mode);
void rt_pin_write(rt_base_t pin, rt_base_t value);
rt_base_t rt_pin_read(rt_base_t pin);

/* ---- Mutex ---- */
typedef void *rt_mutex_t;
rt_mutex_t rt_mutex_create(const char *name, rt_uint8_t flag);
rt_err_t rt_mutex_take(rt_mutex_t mutex, rt_int32_t timeout);
rt_err_t rt_mutex_release(rt_mutex_t mutex);
rt_err_t rt_mutex_delete(rt_mutex_t mutex);

/* ---- Semaphore ---- */
typedef void *rt_sem_t;
rt_sem_t rt_sem_create(const char *name, rt_uint32_t value, rt_uint8_t flag);
rt_err_t rt_sem_take(rt_sem_t sem, rt_int32_t timeout);
rt_err_t rt_sem_release(rt_sem_t sem);
rt_err_t rt_sem_delete(rt_sem_t sem);

/* ---- Thread ---- */
typedef void *rt_thread_t;
rt_thread_t rt_thread_create(const char *name, void (*entry)(void*),
                             void *param, rt_uint32_t stack_size,
                             rt_uint8_t priority, rt_uint32_t tick);
rt_err_t rt_thread_startup(rt_thread_t thread);

/* ---- Sensor framework (optional, some drivers use it) ---- */
#define RT_SENSOR_UNIT_MLUX   0
#define RT_SENSOR_UNIT_DCELSIUS 1

#endif /* __RTTHREAD_H__ */
