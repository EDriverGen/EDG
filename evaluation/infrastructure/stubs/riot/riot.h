/*
 * RIOT OS unified stub header for DriverGen stub compilation.
 * Covers: periph I2C, periph GPIO, ztimer, xtimer, mutex, thread, log.
 */
#ifndef RIOT_STUB_H
#define RIOT_STUB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Base types ─────────────────────────────────────────── */
typedef int16_t  kernel_pid_t;
typedef void *(*thread_task_func_t)(void *);

/* ── errno-style return codes ───────────────────────────── */
#ifndef EIO
#define EIO      5
#endif
#ifndef ENXIO
#define ENXIO    6
#endif
#ifndef EINVAL
#define EINVAL   22
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT 110
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif
#ifndef EAGAIN
#define EAGAIN   11
#endif

/* ── I2C ────────────────────────────────────────────────── */
typedef uint_fast8_t i2c_t;

#define I2C_DEV(x) ((i2c_t)(x))
/* Sentinel for "no/invalid I2C device", mirrors periph/i2c.h in upstream
 * RIOT OS.  Needed because generated adapters use it as the error marker. */
#ifndef I2C_UNDEF
#define I2C_UNDEF  ((i2c_t)0xFF)
#endif

typedef enum {
    I2C_SPEED_LOW       = 0,
    I2C_SPEED_NORMAL    = 1,
    I2C_SPEED_FAST      = 2,
    I2C_SPEED_FAST_PLUS = 3,
    I2C_SPEED_HIGH      = 4,
} i2c_speed_t;

typedef enum {
    I2C_ADDR10  = 0x01,
    I2C_REG16   = 0x02,
    I2C_NOSTOP  = 0x04,
    I2C_NOSTART = 0x08,
} i2c_flags_t;

void i2c_init(i2c_t dev);
void i2c_acquire(i2c_t dev);
void i2c_release(i2c_t dev);

int i2c_read_reg(i2c_t dev, uint16_t addr, uint16_t reg,
                 void *data, uint8_t flags);
int i2c_read_regs(i2c_t dev, uint16_t addr, uint16_t reg,
                  void *data, size_t len, uint8_t flags);
int i2c_read_byte(i2c_t dev, uint16_t addr,
                  void *data, uint8_t flags);
int i2c_read_bytes(i2c_t dev, uint16_t addr,
                   void *data, size_t len, uint8_t flags);

int i2c_write_byte(i2c_t dev, uint16_t addr,
                   uint8_t data, uint8_t flags);
int i2c_write_bytes(i2c_t dev, uint16_t addr,
                    const void *data, size_t len, uint8_t flags);
int i2c_write_reg(i2c_t dev, uint16_t addr, uint16_t reg,
                  uint8_t data, uint8_t flags);
int i2c_write_regs(i2c_t dev, uint16_t addr, uint16_t reg,
                   const void *data, size_t len, uint8_t flags);

/* ── GPIO ───────────────────────────────────────────────── */
typedef unsigned int gpio_t;
typedef void (*gpio_cb_t)(void *);

#define GPIO_PIN(x, y) ((gpio_t)((x) << 8 | (y)))
#define GPIO_UNDEF     ((gpio_t)0xFFFFFFFF)

typedef enum {
    GPIO_IN    = 0,
    GPIO_IN_PD = 1,
    GPIO_IN_PU = 2,
    GPIO_OUT   = 3,
    GPIO_OD    = 4,
    GPIO_OD_PU = 5,
} gpio_mode_t;

typedef enum {
    GPIO_FALLING = 0,
    GPIO_RISING  = 1,
    GPIO_BOTH    = 2,
} gpio_flank_t;

int gpio_init(gpio_t pin, gpio_mode_t mode);
int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank,
                  gpio_cb_t cb, void *arg);
bool gpio_read(gpio_t pin);
void gpio_set(gpio_t pin);
void gpio_clear(gpio_t pin);
void gpio_toggle(gpio_t pin);
void gpio_write(gpio_t pin, bool value);

/* ── SPI ────────────────────────────────────────────────── */
typedef uint_fast8_t spi_t;
typedef unsigned int spi_cs_t;

#define SPI_DEV(x) ((spi_t)(x))
#define SPI_CS_UNDEF ((spi_cs_t)0xFFFFFFFF)
#define SPI_HWCS(x)  ((spi_cs_t)(x))
#ifndef SPI_UNDEF
#define SPI_UNDEF ((spi_t)0xFF)
#endif

typedef enum {
    SPI_MODE_0 = 0,
    SPI_MODE_1 = 1,
    SPI_MODE_2 = 2,
    SPI_MODE_3 = 3,
} spi_mode_t;

typedef enum {
    SPI_CLK_100KHZ = 100000,
    SPI_CLK_400KHZ = 400000,
    SPI_CLK_1MHZ   = 1000000,
    SPI_CLK_5MHZ   = 5000000,
    SPI_CLK_10MHZ  = 10000000,
} spi_clk_t;

void spi_init(spi_t bus);
void spi_init_cs(spi_t bus, spi_cs_t cs);
int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk);
void spi_release(spi_t bus);
void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont,
                        const void *out, void *in, size_t len);
uint8_t spi_transfer_byte(spi_t bus, spi_cs_t cs, bool cont, uint8_t out);
void spi_transfer_regs(spi_t bus, spi_cs_t cs,
                       uint8_t reg, const void *out, void *in, size_t len);
uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out);

/* ── UART ───────────────────────────────────────────────── */
typedef unsigned int uart_t;

#define UART_DEV(x) ((uart_t)(x))
#define UART_UNDEF  ((uart_t)0xFFFFFFFF)

typedef void (*uart_rx_cb_t)(void *arg, uint8_t data);
typedef void (*uart_event_cb_t)(void *arg, int event);

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN = 1,
    UART_PARITY_ODD  = 2,
    UART_PARITY_MARK = 3,
    UART_PARITY_SPACE = 4,
} uart_parity_t;

typedef enum {
    UART_DATA_BITS_5 = 5,
    UART_DATA_BITS_6 = 6,
    UART_DATA_BITS_7 = 7,
    UART_DATA_BITS_8 = 8,
} uart_data_bits_t;

typedef enum {
    UART_STOP_BITS_1 = 1,
    UART_STOP_BITS_2 = 2,
} uart_stop_bits_t;

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg);
int uart_mode(uart_t uart, uart_data_bits_t data_bits,
              uart_parity_t parity, uart_stop_bits_t stop_bits);
void uart_write(uart_t uart, const uint8_t *data, size_t len);
void uart_poweron(uart_t uart);
void uart_poweroff(uart_t uart);
int uart_read(uart_t uart, uint8_t *data, size_t len);

#define UART_OK    0
#define UART_NODEV (-1)
#define UART_NOBAUD (-2)
#define UART_INTERR (-3)
#define UART_NOMODE (-4)

/* ── ztimer (preferred) ─────────────────────────────────── */
typedef struct {
    uint8_t dummy;
} ztimer_clock_t;

typedef struct {
    uint8_t dummy;
} ztimer_t;

extern ztimer_clock_t *ZTIMER_USEC;
extern ztimer_clock_t *ZTIMER_MSEC;
extern ztimer_clock_t *ZTIMER_SEC;

void ztimer_sleep(ztimer_clock_t *clock, uint32_t duration);
void ztimer_spin(ztimer_clock_t *clock, uint32_t duration);
uint32_t ztimer_now(ztimer_clock_t *clock);
void ztimer_set(ztimer_clock_t *clock, ztimer_t *timer, uint32_t offset);
void ztimer_periodic_wakeup(ztimer_clock_t *clock, uint32_t *last_wakeup, uint32_t period);

/* ── xtimer (compatibility) ────────────────────────── */
void xtimer_usleep(uint32_t microseconds);
void xtimer_sleep(uint32_t seconds);
void xtimer_msleep(uint32_t milliseconds);
uint32_t xtimer_now_usec(void);

/* ── Mutex ──────────────────────────────────────────────── */
typedef struct {
    void *queue_next;
} list_node_t;

typedef struct {
    list_node_t queue;
} mutex_t;

#define MUTEX_INIT          { { NULL } }
#define MUTEX_INIT_LOCKED   { { (void *)1 } }

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
int mutex_trylock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

/* ── Thread ─────────────────────────────────────────────── */
#define THREAD_STACKSIZE_DEFAULT  2048
#define THREAD_PRIORITY_MAIN      7
#define THREAD_CREATE_STACKTEST   (1 << 0)

kernel_pid_t thread_create(char *stack, int stacksize,
                           uint8_t priority, int flags,
                           thread_task_func_t task_func,
                           void *arg, const char *name);
void thread_yield(void);
kernel_pid_t thread_getpid(void);

/* ── Logging ────────────────────────────────────────────── */
enum {
    LOG_NONE    = 0,
    LOG_ERROR   = 1,
    LOG_WARNING = 2,
    LOG_INFO    = 3,
    LOG_DEBUG   = 4,
    LOG_ALL     = 5
};

int printf(const char *fmt, ...);

#define LOG(level, ...) printf(__VA_ARGS__)

/* ── Debug ──────────────────────────────────────────────── */
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG 0
#endif
#define DEBUG(...) do { if (ENABLE_DEBUG) { printf(__VA_ARGS__); } } while (0)

/* ── Memory ─────────────────────────────────────────────── */
void *malloc(size_t size);
void free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* RIOT_STUB_H */
