# Raw RTOS/Bus Pack: riot / uart_polling

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/riot/drivers/include/periph/uart.h`

```c
/*
 * Copyright (C) 2014-2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

/**
 * @defgroup    drivers_periph_uart UART
 * @ingroup     drivers_periph
 * @brief       Low-level UART peripheral driver
 *
 * This is a basic UART (Universal Asynchronous Receiver Transmitter) interface
 * to allow platform independent access to the MCU's serial communication
 * abilities. This interface is intentionally designed to be as simple as
 * possible, to allow for easy implementation and maximum portability.
 *
 * The simple interface provides capabilities to initialize and configure
 * the serial communication module, which automatically enables for receiving
 * data, as well as writing data to the UART port, which means transmitting
 * data. The UART device and the corresponding pins need to be mapped in
 * `RIOT/boards/ * /include/periph_conf.h`. Furthermore, you need to select the
 * symbol rate for initialization which is typically {9600, 19200, 38400, 57600,
 * 115200} baud. Additionally, you should register a callback function that is
 * executed in interrupt context when data is being received. The driver will
 * then read the received data byte, call the registered callback function and
 * pass the received data to it via its argument. The interface enforces the
 * receiving to be implemented in an interrupt driven mode. Thus, you never know
 * how many bytes are going to be received and might want to handle that in your
 * specific callback function. The transmit function can be implemented in any
 * way. You can also configure parity, the number of data and stop bits, i.e.
 * such combinations as 8-E-1, 7-N-2 etc. 8-N-1 mode is set by default.
 *
 * By default the @p UART_DEV(0) device of each board is initialized and mapped
 * to STDIO in RIOT which is used for standard input/output functions like
 * `printf()` or `puts()`.
 *
 * # (Low-) Power Implications
 *
 * After initialization, the UART peripheral **should** be powered on and
 * active. The UART can later be explicitly put to sleep and woken up by calling
 * the uart_poweron() and uart_poweroff() functions. Once woken up using
 * uart_poweron(), the UART **should** transparently continue it's previously
 * configured operation.
 *
 * While the UART is active, the implementation might need to block certain
 * power states.
 *
 * @{
 *
 * @file
 * @brief       Low-level UART peripheral driver interface definition
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "periph_cpu.h"
#include "periph_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Threshold under which polling transfers are used instead of DMA
 *          TODO: determine at run-time based on baudrate
 */
#ifndef CONFIG_UART_DMA_THRESHOLD_BYTES
#define CONFIG_UART_DMA_THRESHOLD_BYTES 8
#endif

/**
 * @brief   Define default UART type identifier
 */
#ifndef HAVE_UART_T
typedef uint_fast8_t uart_t;
#endif

/**
 * @brief   Default UART undefined value
 */
#ifndef UART_UNDEF
#define UART_UNDEF          (UINT_FAST8_MAX)
#endif

/**
 * @brief   Default UART device access macro
 */
#ifndef UART_DEV
#define UART_DEV(x)         (x)
#endif

/**
 * @brief   Signature for receive interrupt callback
 *
 * @param[in] arg           context to the callback (optional)
 * @param[in] data          the byte that was received
 */
typedef void(*uart_rx_cb_t)(void *arg, uint8_t data);

/**
 * @brief   Signature for receive start condition interrupt callback
 *
 * @param[in] arg           context to the callback (optional)
 */
typedef void(*uart_rxstart_cb_t)(void *arg);

/**
 * @brief   Interrupt context for a UART device
 */
#ifndef HAVE_UART_ISR_CTX_T
typedef struct {
    uart_rx_cb_t rx_cb;     /**< data received interrupt callback */
    void *arg;              /**< argument to data received callback */
#ifdef MODULE_PERIPH_UART_RXSTART_IRQ
    uart_rxstart_cb_t rxs_cb;   /**< start condition received interrupt callback */
    void *rxs_arg;          /**< argument to start condition received callback */
#endif
} uart_isr_ctx_t;
#endif

/**
 * @brief       Possible UART return values
 *
 * @deprecated  Use the errno constants directly instead.
 */
enum {
    UART_OK         =  0,           /**< everything in order */
    UART_NODEV      = -ENODEV,      /**< invalid UART device given */
    UART_NOBAUD     = -ENOTSUP,     /**< given symbol rate is not applicable */
    UART_NOMODE     = -ENOTSUP,     /**< given mode is not applicable */
    UART_INTERR     = -EIO,         /**< all other internal errors */
};

/**
 * @brief   Definition of possible parity modes
 */
#ifndef HAVE_UART_PARITY_T
typedef enum {
   UART_PARITY_NONE,   /**< no parity */
   UART_PARITY_EVEN,   /**< even parity */
   UART_PARITY_ODD,    /**< odd parity */
   UART_PARITY_MARK,   /**< mark parity */
   UART_PARITY_SPACE   /**< space parity */
} uart_parity_t;
#endif

/**
 * @brief   Definition of possible data bits lengths in a UART frame
 */
#ifndef HAVE_UART_DATA_BITS_T
typedef enum {
    UART_DATA_BITS_5,   /**< 5 data bits */
    UART_DATA_BITS_6,   /**< 6 data bits */
    UART_DATA_BITS_7,   /**< 7 data bits */
    UART_DATA_BITS_8,   /**< 8 data bits */
} uart_data_bits_t;
#endif

/**
 * @brief   Definition of possible stop bits lengths in a UART frame
 */
#ifndef HAVE_UART_STOP_BITS_T
typedef enum {
   UART_STOP_BITS_1,   /**< 1 stop bit */
   UART_STOP_BITS_2,   /**< 2 stop bits */
} uart_stop_bits_t;
#endif

/**
 * @brief   Initialize and acquire a given UART device
 *
 * The UART device will be initialized with the following configuration:
 * - 8 data bits
 * - no parity
 * - 1 stop bit
 * - symbol rate as given
 *
 * If no callback parameter is given (rx_cb := NULL), the UART will be
 * initialized in TX only mode.
 *
 * @pre     The caller is not calling `uart_init()` twice without a call of
 *          to @ref uart_poweroff in between.
 *
 * @note    This may block if the UART is already acquired until it is released.
 *          This allows sharing the underlying peripheral to provide other
 *          serial interfaces (e.g. if the peripheral can also provide SPI, I2C,
 *          etc.)
 *
 * @param[in] uart          UART device to initialize
 * @param[in] baud          desired symbol rate in baud
 * @param[in] rx_cb         receive callback, executed in interrupt context once
 *                          for every byte that is received (RX buffer filled),
 *                          set to NULL for TX only mode
 * @param[in] arg           optional context passed to the callback functions
 *
 * @retval  0               Success
 * @retval  -ENODEV         Invalid UART device
 * @retval  -ENOTSUP        Unsupported symbol rate
 * @retval  <0              On other errors
 */
int uart_init(uart_t uart, uint32_t baud, uart_rx_cb_t rx_cb, void *arg);

#if defined(MODULE_PERIPH_UART_RECONFIGURE) || DOXYGEN
/**
 * @brief   Change the pins of the given UART back to plain GPIO functionality
 *
 * The pin mux of the RX and TX pins of the bus will be changed back to
 * default (GPIO) mode and the UART is powered off.
 * This allows to use the UART pins for another function and return to UART
 * functionality again by calling @ref uart_init_pins
 *
 * If you want the pin to be in a defined state, call @ref gpio_init on it.
 *
 * @note Until this is implemented on all platforms, this requires the
 *       periph_uart_reconfigure feature to be used.
 *
 * @param[in] uart      the device to de-initialize
 */
void uart_deinit_pins(uart_t uart);

/**
 * @brief   Initialize the used UART pins, i.e. RX and TX
 *
 *
 * After calling uart_init, the pins must be initialized (i.e. uart_init is
 * calling this function internally). In normal cases, this function will not
 * be used. But there are some devices, that use UART bus lines also for other
 * purposes and need the option to dynamically re-configure one or more of the
 * used pins. So they can take control over certain pins and return control back
 * to the UART driver using this function.
 *
 * The pins used are configured in the board's periph_conf.h.
 *
 * @param[in] uart      UART device the pins are configure for
 */
void uart_init_pins(uart_t uart);

#if DOXYGEN
/**
 * @brief   Get the RX pin of the given UART.
 *
 * @param[in] uart      The device to query
 *
 * @note Until this is implemented on all platforms, this requires the
 *       periph_uart_reconfigure feature to be used.
 *
 * @return              The GPIO used for the UART RX line.
 */
gpio_t uart_pin_rx(uart_t uart);

/**
 * @brief   Get the TX pin of the given UART.
 *
 * @param[in] uart      The device to query
 *
 * @note Until this is implemented on all platforms, this requires the
 *       periph_uart_reconfigure feature to be used.
 *
 * @return              The GPIO used for the UART TX line.
 */
gpio_t uart_pin_tx(uart_t uart);

#endif /* DOXYGEN */

/**
 * @brief   Get the CTS pin of the given UART.
 *
 * @param[in] uart      The device to query
 *
 * @note Until this is implemented on all platforms, this requires the
 *       periph_uart_reconfigure feature to be used.
 *
 * @return              The GPIO used for the UART CTS line.
 */
gpio_t uart_pin_cts(uart_t uart);

/**
 * @brief   Get the RTS pin of the given UART.
 *
 * @param[in] uart      The device to query
 *
 * @note Until this is implemented on all platforms, this requires the
 *       periph_uart_reconfigure feature to be used.
 *
 * @return              The GPIO used for the UART RTS line.
 */
gpio_t uart_pin_rts(uart_t uart);

#endif /* MODULE_PERIPH_UART_RECONFIGURE */

#if defined(MODULE_PERIPH_UART_RXSTART_IRQ) || DOXYGEN

/**
 * @brief   Configure the function that will be called when a start condition
 *          is detected.
 *
 *          This will not enable / disable the generation of the RX start
 *          interrupt.
 *
 * @note    You have to add the module `periph_uart_rxstart_irq` to your project
 *          to enable this function
 *
 * @param[in] uart      The device to configure
 * @param[in] cb        The function called when a start condition is detected
 * @param[in] arg       Optional function argument
 */
void uart_rxstart_irq_configure(uart_t uart, uart_rxstart_cb_t cb, void *arg);

/**
 * @brief   Enable the RX start interrupt.
 *
 * @note    You have to add the module `periph_uart_rxstart_irq` to your project
 *          to enable this function
 *
 * @param[in] uart      The device to configure
 */
void uart_rxstart_irq_enable(uart_t uart);

/**
 * @brief   Disable the RX start interrupt.
 *
 * @note    You have to add the module `periph_uart_rxstart_irq` to your project
 *          to enable this function
 *
 * @param[in] uart      The device to configure
 */
void uart_rxstart_irq_disable(uart_t uart);
#endif /* MODULE_PERIPH_UART_RXSTART_IRQ */

#if defined(MODULE_PERIPH_UART_COLLISION) || DOXYGEN
/**
 * @brief   Enables collision detection check of the UART.
 *          This assumes the UART is connected to a bus where RX and TX are
 *          connected. After each sent byte it is checked whether the same
 *          byte could be received.
 *
 *          This disables the RX interrupt.
 *
 * @note    You have to add the module `periph_uart_rxstart_irq` to your project
 *          to enable this function
 *
 * @param[in] uart      The device to configure
 */
void uart_collision_detect_enable(uart_t uart);
/**
 * @brief   Disables collision detection check of the UART.
 *
 *          If an RX interrupt was configured before, it is enabled again.
 *
 * @note    You have to add the module `periph_uart_rxstart_irq` to your project
 *          to enable this function
 *
 * @param[in] uart      The device to configure
 */
void uart_collision_detect_disable(uart_t uart);

/**
 * @brief   Disables collision detection check of the UART.
 *
 *          If an RX interrupt was configured before, it is enabled again.
 *
 * @note    You have to add the module `periph_uart_rxstart_irq` to your project
 *          to enable this function
 *
 * @param[in] uart      The device to probe
 *
 * @return              true if a collision occurred during the last transder
 */
bool uart_collision_detected(uart_t uart);
#endif /* MODULE_PERIPH_UART_COLLISION */

/**
 * @brief   Setup parity, data and stop bits for a given UART device
 *
 * @param[in] uart          UART device to configure
 * @param[in] data_bits     number of data bits in a UART frame
 * @param[in] parity        parity mode
 * @param[in] stop_bits     number of stop bits in a UART frame
 *
 * @retval  0               Success
 * @retval  -ENOTSUP        Given configuration not supported
 * @retval  <0              Other error
 */
int uart_mode(uart_t uart, uart_data_bits_t data_bits, uart_parity_t parity,
              uart_stop_bits_t stop_bits);

/**
 * @brief   Write data from the given buffer to the specified UART device
 *
 * This function is blocking, as it will only return after @p len bytes from the
 * given buffer have been send. The way this data is send is up to the
 * implementation: active waiting, interrupt driven, DMA, etc.
 *
 * @param[in] uart          UART device to use for transmission
 * @param[in] data          data buffer to send
 * @param[in] len           number of bytes to send
 *
 */
void uart_write(uart_t uart, const uint8_t *data, size_t len);

/**
 * @brief   Power on and acquire the given UART device
 *
 * The UART will resume with the configuration it was most recently configured
 * with.
 *
 * @param[in] uart          the UART device to power on
 */
void uart_poweron(uart_t uart);

/**
 * @brief Power off and release the given UART device
 *
 * @param[in] uart          the UART device to power off
 */
void uart_poweroff(uart_t uart);

/**
 * @brief Enable the TX line one the given UART
 *
 * @note requires the `periph_uart_tx_ondemand` feature
 *
 * @param[in] uart          the UART device start TX on
 */
void uart_enable_tx(uart_t uart);

/**
 * @brief Disable the TX line one the given UART
 *
 * @note requires the `periph_uart_tx_ondemand` feature
 *
 * @param[in] uart          the UART device to stop TX on
 */
void uart_disable_tx(uart_t uart);

#ifdef __cplusplus
}
#endif

/** @} */
```

## Source: `data/rtos/riot/sys/include/xtimer.h`

```c
/*
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 * Copyright (C) 2016 Eistec AB
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

/**
 * @defgroup    sys_xtimer xtimer high level timer abstraction layer (deprecated)
 * @ingroup     sys
 * @brief       Provides a high level timer module to register
 *              timers, get current system time, and let a thread sleep for
 *              a certain amount of time.
 *
 * The implementation takes one low-level timer and multiplexes it.
 *
 * Insertion and removal of timers has O(n) complexity with (n) being the
 * number of active timers.  The reason for this is that multiplexing is
 * realized by next-first singly linked lists.
 *
 * @deprecated  xtimer has been deprecated in favor of the @ref sys_ztimer
 * @note        With @ref sys_ztimer_xtimer_compat a compatibility wrapper is
 *              provided that in the vast majority of cases can function as a
 *              drop-in replacement. This compatibility wrapper is expected to
 *              replace `xtimer` in the near future and ensure that code still
 *              depending on the `xtimer` API continues to function.
 * @details     Note that at least for long running timers, using
 *              @ref sys_ztimer instead of the compatibility layer can yield
 *              lower clock drift and lower power consumption compared to
 *              using the compatibility layer. For details on how to achieve
 *              lower clock drift and lower power consumption please consult the
 *              @ref sys_ztimer documentation.
 *
 * @{
 * @file
 * @brief   xtimer interface definitions
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 * @author  Joakim Nohlgård <joakim.nohlgard@eistec.se>
 */

/**
 * @brief This macro is used to check whether internal headers have been
 * included via the public header
 */
#define XTIMER_H

#include <stdbool.h>
#include <stdint.h>

#include "modules.h"
#include "timex.h"
#ifdef MODULE_CORE_MSG
#include "msg.h"
#endif /* MODULE_CORE_MSG */
#include "mutex.h"
#include "sched.h"
#include "rmutex.h"

#if IS_USED(MODULE_ZTIMER64_XTIMER_COMPAT)
#include "ztimer64/xtimer_compat.h" /* IWYU pragma: export */
#elif IS_USED(MODULE_ZTIMER_XTIMER_COMPAT)
#include "ztimer/xtimer_compat.h" /* IWYU pragma: export */
#else
#include "board.h"
#if !IS_USED(MODULE_XTIMER_ON_ZTIMER)
#include "periph_conf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief xtimer timestamp (64 bit)
 *
 * @note This is a struct in order to make the xtimer API type strict
 */
typedef struct {
    uint64_t ticks64;       /**< Tick count */
} xtimer_ticks64_t;

/**
 * @brief xtimer timestamp (32 bit)
 *
 * @note This is a struct in order to make the xtimer API type strict
 */
typedef struct {
    uint32_t ticks32;       /**< Tick count */
} xtimer_ticks32_t;

/**
 * @brief xtimer callback type
 */
typedef void (*xtimer_callback_t)(void*);

/**
 * @brief xtimer timer structure
 */
typedef struct xtimer {
    struct xtimer *next;         /**< reference to next timer in timer lists */
    uint32_t offset;             /**< lower 32bit offset time */
    uint32_t long_offset;        /**< upper 32bit offset time */
    uint32_t start_time;         /**< lower 32bit absolute start time */
    uint32_t long_start_time;    /**< upper 32bit absolute start time */
    xtimer_callback_t callback;  /**< callback function to call when timer
                                     expires */
    void *arg;                   /**< argument to pass to callback function */
} xtimer_t;

/**
 * @brief get the current system time as 32bit time stamp value
 *
 * @note    Overflows 2**32 ticks, thus returns xtimer_now64() % 32,
 *          but is cheaper.
 *
 * @return  current time as 32bit time stamp
 */
static inline xtimer_ticks32_t xtimer_now(void);

/**
 * @brief get the current system time as 64bit time stamp
 *
 * @return  current time as 64bit time stamp
 */
static inline xtimer_ticks64_t xtimer_now64(void);

/**
 * @brief get the current system time into a timex_t
 *
 * @param[out] out  pointer to timex_t the time will be written to
 */
void xtimer_now_timex(timex_t *out);

/**
 * @brief get the current system time in microseconds since start
 *
 * This is a convenience function for @c xtimer_usec_from_ticks(xtimer_now())
 */
static inline uint32_t xtimer_now_usec(void);

/**
 * @brief get the current system time in microseconds since start
 *
 * This is a convenience function for @c xtimer_usec_from_ticks64(xtimer_now64())
 */
static inline uint64_t xtimer_now_usec64(void);

/**
 * @brief xtimer initialization function
 *
 * This sets up xtimer. Has to be called once at system boot.
 * If @ref auto_init is enabled, it will call this for you.
 */
void xtimer_init(void);

/**
 * @brief Pause the execution of a thread for some seconds
 *
 * When called from an ISR, this function will spin and thus block the MCU in
 * interrupt context for the specified amount in *seconds*, so don't *ever* use
 * it there.
 *
 * @param[in] seconds   the amount of seconds the thread should sleep
 */
static inline void xtimer_sleep(uint32_t seconds);

/**
 * @brief Pause the execution of a thread for some milliseconds
 *
 * @param[in] milliseconds  the amount of milliseconds the thread should sleep
 */
static inline void xtimer_msleep(uint32_t milliseconds);

/**
 * @brief Pause the execution of a thread for some microseconds
 *
 * When called from an ISR, this function will spin and thus block the MCU for
 * the specified amount in microseconds, so only use it there for *very* short
 * periods, e.g., less than XTIMER_BACKOFF converted to µs.
 *
 * @param[in] microseconds  the amount of microseconds the thread should sleep
 */
static inline void xtimer_usleep(uint32_t microseconds);

/**
 * @brief Pause the execution of a thread for some microseconds
 *
 * See xtimer_usleep() for more information.
 *
 * @param[in] microseconds  the amount of microseconds the thread should sleep
 */
static inline void xtimer_usleep64(uint64_t microseconds);

/**
 * @brief Stop execution of a thread for some time, 32bit version
 *
 * When called from an ISR, this function will spin and thus block the MCU for
 * the specified amount, so only use it there for *very* short periods,
 * e.g. less than XTIMER_BACKOFF.
 *
 * @param[in] ticks  number of ticks the thread should sleep
 */
static inline void xtimer_tsleep32(xtimer_ticks32_t ticks);

/**
 * @brief Stop execution of a thread for some time, 64bit version
 *
 * When called from an ISR, this function will spin and thus block the MCU for
 * the specified amount, so only use it there for *very* short periods,
 * e.g. less than XTIMER_BACKOFF.
 *
 * @param[in] ticks  number of ticks the thread should sleep
 */
static inline void xtimer_tsleep64(xtimer_ticks64_t ticks);

/**
 * @brief Stop execution of a thread for some time, blocking
 *
 * This function will spin-block, so only use it *very* short periods.
 *
 * @param[in] ticks  the number of xtimer ticks the thread should spin for
 */
static inline void xtimer_spin(xtimer_ticks32_t ticks);

/**
 * @brief will cause the calling thread to be suspended until the absolute
 * time (@p last_wakeup + @p period).
 *
 * When the function returns, @p last_wakeup is set to
 * (@p last_wakeup + @p period).
 *
 * This function can be used to create periodic wakeups.
 * @c last_wakeup should be set to xtimer_now() before first call of the
 * function.
 *
 * If the result of (@p last_wakeup + @p period) would be in the past, the function
 * sets @p last_wakeup to @p last_wakeup + @p period and returns immediately.
 *
 * @param[in] last_wakeup   base time stamp for the wakeup
 * @param[in] period        time in microseconds that will be added to last_wakeup
 */
static inline void xtimer_periodic_wakeup(xtimer_ticks32_t *last_wakeup, uint32_t period);

/**
 * @brief Set a timer that wakes up a thread
 *
 * This function sets a timer that will wake up a thread when the timer has
 * expired.
 *
 * @param[in] timer         timer struct to work with.
 * @param[in] offset        microseconds from now
 * @param[in] pid           pid of the thread that will be woken up
 */
static inline void xtimer_set_wakeup(xtimer_t *timer, uint32_t offset, kernel_pid_t pid);

/**
 * @brief Set a timer that wakes up a thread, 64bit version
 *
 * This function sets a timer that will wake up a thread when the timer has
 * expired.
 *
 * @param[in] timer         timer struct to work with.
 * @param[in] offset        microseconds from now
 * @param[in] pid           pid of the thread that will be woken up
 */
static inline void xtimer_set_wakeup64(xtimer_t *timer, uint64_t offset, kernel_pid_t pid);

/**
 * @brief Set a timer to execute a callback at some time in the future
 *
 * Expects timer->callback to be set.
 *
 * The callback specified in the timer struct will be executed @p offset
 * ticks in the future.
 *
 * @warning BEWARE! Callbacks from xtimer_set() are being executed in interrupt
 * context (unless offset < XTIMER_BACKOFF converted to µs).
 * DON'T USE THIS FUNCTION unless you know *exactly* what that means.
 *
 * @param[in] timer     the timer structure to use.
 * @param[in] offset    time in microseconds from now specifying that timer's
 *                      callback's execution time
 */
static inline void xtimer_set(xtimer_t *timer, uint32_t offset);

/**
 * @brief Set a timer to execute a callback at some time in the future, 64bit
 * version
 *
 * Expects timer->callback to be set.
 *
 * The callback specified in the timer struct will be executed @p offset_usec
 * microseconds in the future.
 *
 * @warning BEWARE! Callbacks from xtimer_set() are being executed in interrupt
 * context (unless offset < XTIMER_BACKOFF converted to µs).
 * DON'T USE THIS FUNCTION unless you know *exactly* what that means.
 *
 * @param[in] timer       the timer structure to use.
 * @param[in] offset_us   time in microseconds from now specifying that timer's
 *                        callback's execution time
 */
static inline void xtimer_set64(xtimer_t *timer, uint64_t offset_us);

/**
 * @brief remove a timer
 *
 * @note this function runs in O(n) with n being the number of active timers
 *
 * @param[in] timer ptr to timer structure that will be removed
 */
void xtimer_remove(xtimer_t *timer);

/**
 * @brief state if an xtimer is currently set (waiting to be expired)
 *
 * @param[in] timer ptr to timer structure to work on
 */
static inline bool xtimer_is_set(const xtimer_t *timer);

/**
 * @brief Convert microseconds to xtimer ticks
 *
 * @param[in] usec  microseconds
 *
 * @return xtimer time stamp
 */
static inline xtimer_ticks32_t xtimer_ticks_from_usec(uint32_t usec);

/**
 * @brief Convert microseconds to xtimer ticks, 64 bit version
 *
 * @param[in] usec  microseconds
 *
 * @return xtimer time stamp
 */
static inline xtimer_ticks64_t xtimer_ticks_from_usec64(uint64_t usec);

/**
 * @brief Convert xtimer ticks to microseconds
 *
 * @param[in] ticks  xtimer time stamp
 *
 * @return microseconds
 */
static inline uint32_t xtimer_usec_from_ticks(xtimer_ticks32_t ticks);

/**
 * @brief Convert xtimer ticks to microseconds, 64 bit version
 *
 * @param[in] ticks  xtimer time stamp
 *
 * @return microseconds
 */
static inline uint64_t xtimer_usec_from_ticks64(xtimer_ticks64_t ticks);

/**
 * @brief Create an xtimer time stamp
 *
 * @param[in] ticks  number of xtimer ticks
 *
 * @return xtimer time stamp
 */
static inline xtimer_ticks32_t xtimer_ticks(uint32_t ticks);

/**
 * @brief Create an xtimer time stamp, 64 bit version
 *
 * @param[in] ticks  number of xtimer ticks
 *
 * @return xtimer time stamp
 */
static inline xtimer_ticks64_t xtimer_ticks64(uint64_t ticks);

/**
 * @brief Compute difference between two xtimer time stamps
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a - @p b
 */
static inline xtimer_ticks32_t xtimer_diff(xtimer_ticks32_t a, xtimer_ticks32_t b);

/**
 * @brief Compute difference between two xtimer time stamps, 64 bit version
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a - @p b
 */
static inline xtimer_ticks64_t xtimer_diff64(xtimer_ticks64_t a, xtimer_ticks64_t b);

/**
 * @brief Compute 32 bit difference between two 64 bit xtimer time stamps
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a - @p b cast truncated to 32 bit
 */
static inline xtimer_ticks32_t xtimer_diff32_64(xtimer_ticks64_t a, xtimer_ticks64_t b);

/**
 * @brief Compare two xtimer time stamps
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a < @p b
 */
static inline bool xtimer_less(xtimer_ticks32_t a, xtimer_ticks32_t b);

/**
 * @brief Compare two xtimer time stamps, 64 bit version
 *
 * @param[in] a  left operand
 * @param[in] b  right operand
 *
 * @return @p a < @p b
 */
static inline bool xtimer_less64(xtimer_ticks64_t a, xtimer_ticks64_t b);

/**
 * @brief lock a mutex but with timeout
 *
 * @param[in]    mutex  mutex to lock
 * @param[in]    us     timeout in microseconds relative
 *
 * @return       0, when returned after mutex was locked
 * @return       -1, when the timeout occurred
 */
int xtimer_mutex_lock_timeout(mutex_t *mutex, uint64_t us);

/**
 * @brief lock a rmutex but with timeout
 *
 * @param[in]    rmutex  rmutex to lock
 * @param[in]    us     timeout in microseconds relative
 *
 * @return       0, when returned after rmutex was locked
 * @return       -1, when the timeout occurred
 */
int xtimer_rmutex_lock_timeout(rmutex_t *rmutex, uint64_t us);

#if defined(MODULE_CORE_THREAD_FLAGS) || defined(DOXYGEN)

/**
 * @brief    Set timeout thread flag after @p timeout
 *
 * This function will set THREAD_FLAG_TIMEOUT on the current thread after @p
 * timeout usec have passed.
 *
 * @param[in]   t       timer struct to use
 * @param[in]   timeout timeout in usec
 */
void xtimer_set_timeout_flag(xtimer_t *t, uint32_t timeout);

/**
 * @brief    Set timeout thread flag after @p timeout
 *
 * See xtimer_set_timeout_flag() for more information.
 *
 * @param[in]   t       timer struct to use
 * @param[in]   timeout timeout in usec
 */
void xtimer_set_timeout_flag64(xtimer_t *t, uint64_t timeout);
#endif

/**
 * @brief   Get remaining time of timer
 *
 * @param[in]   timer   timer struct to use
 *
 * @returns time in usec until timer triggers
 * @returns 0 if timer is not set (or has already passed)
 */
uint64_t xtimer_left_usec(const xtimer_t *timer);

#if defined(MODULE_CORE_MSG) || defined(DOXYGEN)
/**
 * @brief Set a timer that sends a message
 *
 * This function sets a timer that will send a message @p offset ticks
 * from now.
 *
 * The message struct specified by msg parameter will not be copied, e.g., it
 * needs to point to valid memory until the message has been delivered.
 *
 * @param[in] timer         timer struct to work with.
 * @param[in] offset        microseconds from now
 * @param[in] msg           ptr to msg that will be sent
 * @param[in] target_pid    pid the message will be sent to
 */
static inline void xtimer_set_msg(xtimer_t *timer, uint32_t offset, msg_t *msg, kernel_pid_t target_pid);

/**
 * @brief Set a timer that sends a message, 64bit version
 *
 * This function sets a timer that will send a message @p offset microseconds
 * from now.
 *
 * The message struct specified by msg parameter will not be copied, e.g., it
 * needs to point to valid memory until the message has been delivered.
 *
 * @param[in] timer         timer struct to work with.
 * @param[in] offset        microseconds from now
 * @param[in] msg           ptr to msg that will be sent
 * @param[in] target_pid    pid the message will be sent to
 */
static inline void xtimer_set_msg64(xtimer_t *timer, uint64_t offset, msg_t *msg, kernel_pid_t target_pid);

/**
 * @brief receive a message blocking but with timeout
 *
 * @param[out] msg      pointer to a msg_t which will be filled in case of
 *                      no timeout
 * @param[in]  timeout  timeout in microseconds relative
 *
 * @return     < 0 on error, other value otherwise
 */
static inline int xtimer_msg_receive_timeout(msg_t *msg, uint32_t timeout);

/**
 * @brief receive a message blocking but with timeout, 64bit version
 *
 * @param[out] msg      pointer to a msg_t which will be filled in case of no
 *                      timeout
 * @param[in]  timeout  timeout in microseconds relative
 *
 * @return     < 0 on error, other value otherwise
 */
static inline int xtimer_msg_receive_timeout64(msg_t *msg, uint64_t timeout);
#endif

/**
 * @brief xtimer backoff value
 *
 * All timers that are less than XTIMER_BACKOFF ticks in the future will
 * just spin.
 *
 * This is supposed to be defined per-device in e.g., periph_conf.h.
 */
#ifndef XTIMER_BACKOFF
#define XTIMER_BACKOFF 30
#endif

#ifndef XTIMER_ISR_BACKOFF
/**
 * @brief   xtimer IRQ backoff time, in hardware ticks
 *
 * When scheduling the next IRQ, if it is less than the backoff time
 * in the future, just spin.
 *
 * This is supposed to be defined per-device in e.g., periph_conf.h.
 */
#define XTIMER_ISR_BACKOFF 20
#endif

/*
 * Default xtimer configuration
 */
#ifndef XTIMER_DEV
/**
 * @brief Underlying hardware timer device to assign to xtimer
 */
#define XTIMER_DEV TIMER_DEV(0)
/**
 * @brief Underlying hardware timer channel to assign to xtimer
 */
#define XTIMER_CHAN (0)

#if (TIMER_0_MAX_VALUE) == 0xfffffful
#define XTIMER_WIDTH (24)
#elif (TIMER_0_MAX_VALUE) == 0xffff
#define XTIMER_WIDTH (16)
#endif

#endif

#ifndef XTIMER_WIDTH
/**
 * @brief xtimer timer width
 *
 * This value specifies the width (in bits) of the hardware timer used by xtimer.
 * Default is 32.
 */
#define XTIMER_WIDTH (32)
#endif

#if (XTIMER_WIDTH != 32) || DOXYGEN
/**
 * @brief xtimer timer mask
 *
 * This value specifies the mask relative to 0xffffffff that the used timer
 * counts to, e.g., 0xffffffff & ~TIMER_MAXVALUE.
 *
 * For a 16bit timer, the mask would be 0xFFFF0000, for a 24bit timer, the mask
 * would be 0xFF000000.
 */
#define XTIMER_MASK ((0xffffffff >> XTIMER_WIDTH) << XTIMER_WIDTH)
#else
#define XTIMER_MASK (0)
#endif

/**
 * @brief  Base frequency of xtimer is 1 MHz
 */
#define XTIMER_HZ_BASE (1000000ul)

#if !defined(XTIMER_HZ) && !defined(MODULE_XTIMER_ON_ZTIMER)
/**
 * @brief  Frequency of the underlying hardware timer
 */
#define XTIMER_HZ XTIMER_HZ_BASE
#endif

#if !defined(XTIMER_SHIFT) && !defined(MODULE_XTIMER_ON_ZTIMER)
#if (XTIMER_HZ == 32768ul)
/* No shift necessary, the conversion is not a power of two and is handled by
 * functions in tick_conversion.h */
#define XTIMER_SHIFT (0)
#elif (XTIMER_HZ == XTIMER_HZ_BASE)
/**
 * @brief   xtimer prescaler value
 *
 * If the underlying hardware timer is running at a power of two multiple of
 * 15625, XTIMER_SHIFT can be used to adjust the difference.
 *
 * For a 1 MHz hardware timer, set XTIMER_SHIFT to 0.
 * For a 2 MHz or 500 kHz, set XTIMER_SHIFT to 1.
 * For a 4 MHz or 250 kHz, set XTIMER_SHIFT to 2.
 * For a 8 MHz or 125 kHz, set XTIMER_SHIFT to 3.
 * For a 16 MHz or 62.5 kHz, set XTIMER_SHIFT to 4.
 * and for 32 MHz, set XTIMER_SHIFT to 5.
 *
 * The direction of the shift is handled by the macros in tick_conversion.h
 */
#define XTIMER_SHIFT (0)
#elif (XTIMER_HZ >> 1 == XTIMER_HZ_BASE) || (XTIMER_HZ << 1 == XTIMER_HZ_BASE)
#define XTIMER_SHIFT (1)
#elif (XTIMER_HZ >> 2 == XTIMER_HZ_BASE) || (XTIMER_HZ << 2 == XTIMER_HZ_BASE)
#define XTIMER_SHIFT (2)
#elif (XTIMER_HZ >> 3 == XTIMER_HZ_BASE) || (XTIMER_HZ << 3 == XTIMER_HZ_BASE)
#define XTIMER_SHIFT (3)
#elif (XTIMER_HZ >> 4 == XTIMER_HZ_BASE) || (XTIMER_HZ << 4 == XTIMER_HZ_BASE)
#define XTIMER_SHIFT (4)
#elif (XTIMER_HZ >> 5 == XTIMER_HZ_BASE) || (XTIMER_HZ << 5 == XTIMER_HZ_BASE)
#define XTIMER_SHIFT (5)
#elif (XTIMER_HZ >> 6 == XTIMER_HZ_BASE) || (XTIMER_HZ << 6 == XTIMER_HZ_BASE)
#define XTIMER_SHIFT (6)
#else
#error "XTIMER_SHIFT cannot be derived for given XTIMER_HZ, verify settings!"
#endif
#elif !defined(MODULE_XTIMER_ON_ZTIMER)
#error "XTIMER_SHIFT is set relative to XTIMER_HZ, no manual define required!"
#endif

#include "xtimer/tick_conversion.h"

#include "xtimer/implementation.h"

#ifdef __cplusplus
}
#endif

#endif /* MODULE_ZTIMER_XTIMER_COMPAT */

/** @} */
```
