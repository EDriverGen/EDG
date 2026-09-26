# Raw RTOS/Bus Pack: riot / gpio_timing

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/riot/drivers/include/periph/gpio.h`

```c
/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

/**
 * @defgroup    drivers_periph_gpio GPIO
 * @ingroup     drivers_periph
 * @brief       Low-level GPIO peripheral driver
 *
 * This is a basic GPIO (General-purpose input/output) interface to allow
 * platform independent access to a MCU's input/output pins. This interface is
 * intentionally designed to be as simple as possible, to allow for easy
 * implementation and maximum portability.
 *
 * The interface provides capabilities to initialize a pin as output-,
 * input- and interrupt pin. With the API you can basically set/clear/toggle the
 * digital signal at the hardware pin when in output mode. Configured as input you can
 * read a digital value that is being applied to the pin externally. When initializing
 * an external interrupt pin, you can register a callback function that is executed
 * in interrupt context once the interrupt condition applies to the pin. Usually you
 * can react to rising or falling signal flanks (or both).
 *
 * In addition the API provides to set standard input/output circuit modes such as
 * e.g. internal push-pull configurations.
 *
 * All modern micro controllers organize their GPIOs in some form of ports,
 * often named 'PA', 'PB', 'PC'..., or 'P0', 'P1', 'P2'..., or similar. Each of
 * these ports is then assigned a number of pins, often 8, 16, or 32. A hardware
 * pin can thus be described by its port/pin tuple. To access a pin, the
 * @p GPIO_PIN(port, pin) macro should be used. For example: If your platform has
 * a pin PB22, it will be port=1 and pin=22. The @p GPIO_PIN macro should be
 * overridden by a MCU, to allow for efficient encoding of the port/pin tuple.
 * For example, on many platforms it is possible to `OR` the pin number with the
 * corresponding ports base register address. This allows for efficient decoding
 * of pin number and base address without the need of any address lookup.
 *
 * In case the driver does not define it, the below macro definition is used to
 * simply map the port/pin tuple to the pin value. In that case, predefined GPIO
 * definitions in `RIOT/boards/ * /include/periph_conf.h` will define the selected
 * GPIO pin.
 *
 * @warning The scalar GPIO pin type `gpio_t` is deprecated and will be
 * replaced by a structured GPIO pin type in a future GPIO API. Therefore,
 * don't use the direct comparison of GPIO pins anymore. Instead, use the
 * inline comparison functions @ref gpio_is_equal and @ref gpio_is_valid.
 *
 * # (Low-) Power Implications
 *
 * On almost all platforms, we can only control the peripheral power state of
 * full ports (i.e. groups of pins), but not for single GPIO pins. Together with
 * CPU specific alternate function handling for pins used by other peripheral
 * drivers, this can make it quite complex to keep track of pins that are
 * currently used at a certain moment. To simplify the implementations (and ease
 * the memory consumption), we expect ports to be powered on (e.g. through
 * peripheral clock gating) when first used and never be powered off again.
 *
 * GPIO driver implementations **should** power on the corresponding port during
 * gpio_init() and gpio_init_int().
 *
 * For external interrupts to work, some platforms may need to block certain
 * power modes (although this is not very likely). This should be done during
 * gpio_init_int().
 *
 * @{
 * @file
 * @brief       Low-level GPIO peripheral driver interface definitions
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#include <limits.h>
#include <stdbool.h>

#include "periph_cpu.h"
#include "periph_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_GPIO_T
/**
 * @brief   GPIO type identifier
 */
typedef unsigned int gpio_t;
#endif

#ifndef GPIO_PIN
/**
 * @brief   Convert (port, pin) tuple to @c gpio_t value
 */
/* Default GPIO macro maps port-pin tuples to the pin value */
#define GPIO_PIN(x,y)       ((gpio_t)((x & 0) | y))
#endif

#ifndef GPIO_UNDEF
/**
 * @brief   GPIO pin not defined
 */
#define GPIO_UNDEF          ((gpio_t)(UINT_MAX))
#endif

/**
 * @brief   Available pin modes
 *
 * Generally, a pin can be configured to be input or output. In output mode, a
 * pin can further be put into push-pull or open drain configuration. Though
 * this is supported by most platforms, this is not always the case, so driver
 * implementations may return an error code if a mode is not supported.
 */
#ifndef HAVE_GPIO_MODE_T
typedef enum {
    GPIO_IN ,               /**< configure as input without pull resistor */
    GPIO_IN_PD,             /**< configure as input with pull-down resistor */
    GPIO_IN_PU,             /**< configure as input with pull-up resistor */
    GPIO_OUT,               /**< configure as output in push-pull mode */
    GPIO_OD,                /**< configure as output in open-drain mode without
                             *   pull resistor */
    GPIO_OD_PU              /**< configure as output in open-drain mode with
                             *   pull resistor enabled */
} gpio_mode_t;
#endif

/**
 * @brief   Definition of possible active flanks for external interrupt mode
 */
#ifndef HAVE_GPIO_FLANK_T
typedef enum {
    GPIO_FALLING = 0,       /**< emit interrupt on falling flank */
    GPIO_RISING = 1,        /**< emit interrupt on rising flank */
    GPIO_BOTH = 2           /**< emit interrupt on both flanks */
} gpio_flank_t;
#endif

/**
 * @brief   Signature of event callback functions triggered from interrupts
 *
 * @param[in] arg       optional context for the callback
 */
typedef void (*gpio_cb_t)(void *arg);

/**
 * @brief   Default interrupt context for GPIO pins
 */
#ifndef HAVE_GPIO_ISR_CTX_T
typedef struct {
    gpio_cb_t cb;           /**< interrupt callback */
    void *arg;              /**< optional argument */
} gpio_isr_ctx_t;
#endif

/**
 * @brief   Initialize the given pin as general purpose input or output
 *
 * When configured as output, the pin state after initialization is undefined.
 * The output pin's state **should** be untouched during the initialization.
 * This behavior can however **not be guaranteed** by every platform.
 *
 * @param[in] pin       pin to initialize
 * @param[in] mode      mode of the pin, see @c gpio_mode_t
 *
 * @return              0 on success
 * @return              -1 on error
 */
int gpio_init(gpio_t pin, gpio_mode_t mode);

#if defined(MODULE_PERIPH_GPIO_IRQ) || defined(DOXYGEN)
/**
 * @brief   Initialize a GPIO pin for external interrupt usage
 *
 * The registered callback function will be called in interrupt context every
 * time the defined flank(s) are detected.
 *
 * The interrupt is activated automatically after the initialization.
 *
 * @note    You have to add the module `periph_gpio_irq` to your project to
 *          enable this function
 *
 * @pre     @p cb must not be NULL
 *
 * @param[in] pin       pin to initialize
 * @param[in] mode      mode of the pin, see @c gpio_mode_t
 * @param[in] flank     define the active flank(s)
 * @param[in] cb        callback that is called from interrupt context
 * @param[in] arg       optional argument passed to the callback
 *
 * @return              0 on success
 * @return              -1 on error
 */
int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank,
                  gpio_cb_t cb, void *arg);

/**
 * @brief   Enable pin interrupt if configured as interrupt source
 *
 * Interrupts that would have occurred after @see gpio_irq_disable
 * was called will be discarded.
 *
 * @note    You have to add the module `periph_gpio_irq` to your project to
 *          enable this function
 *
 * @param[in] pin       the pin to enable the interrupt for
 */
void gpio_irq_enable(gpio_t pin);

/**
 * @brief   Disable the pin interrupt if configured as interrupt source
 *
 * @note    You have to add the module `periph_gpio_irq` to your project to
 *          enable this function
 *
 * @param[in] pin       the pin to disable the interrupt for
 */
void gpio_irq_disable(gpio_t pin);

#endif /* defined(MODULE_PERIPH_GPIO_IRQ) || defined(DOXYGEN) */

/**
 * @brief   Get the current value of the given pin
 *
 * @param[in] pin       the pin to read
 *
 * @retval              false   pin is LOW
 * @retval              true    pin is HIGH
 */
bool gpio_read(gpio_t pin);

/**
 * @brief   Set the given pin to HIGH
 *
 * @param[in] pin       the pin to set
 */
void gpio_set(gpio_t pin);

/**
 * @brief   Set the given pin to LOW
 *
 * @param[in] pin       the pin to clear
 */
void gpio_clear(gpio_t pin);

/**
 * @brief   Toggle the value of the given pin
 *
 * @param[in] pin       the pin to toggle
 */
void gpio_toggle(gpio_t pin);

/**
 * @brief   Set the given pin to the given value
 *
 * @param[in] pin       the pin to set
 * @param[in] value     value to set the pin to, 0 for LOW, HIGH otherwise
 */
void gpio_write(gpio_t pin, bool value);

/**
 * @brief   Test if a GPIO pin is equal to another GPIO pin
 *
 * @param[in] gpio1 First GPIO pin to check
 * @param[in] gpio2 Second GPIO pin to check
 */
static inline int gpio_is_equal(gpio_t gpio1, gpio_t gpio2)
{
    return (gpio1 == gpio2);
}

/**
 * @brief   Test if a GPIO pin is a valid pin and not declared as undefined.
 *
 * @param[in] gpio GPIO pin to check
 */
static inline int gpio_is_valid(gpio_t gpio)
{
    return (gpio != GPIO_UNDEF);
}

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
