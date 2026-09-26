# Raw RTOS/Bus Pack: riot / i2c

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/riot/drivers/include/periph/i2c.h`

```c
/*
 * Copyright (C) 2014-2017 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

/**
 * @defgroup    drivers_periph_i2c I2C
 * @ingroup     drivers_periph
 * @brief       Low-level I2C peripheral driver
 *
 * This interface provides a simple abstraction to use the MCUs I2C peripherals.
 * It provides support for 7-bit and 10-bit addressing and can be used for
 * different kind of register addressing schemes.
 *
 *
 * @section   sec_i2c_usage Usage
 *
 * Example for reading a 8-bit register on a device, using a 10-bit device
 * address and 8-bit register addresses and using a RESTART condition (CAUTION:
 * this example does not check any return values...):
 *
 * ```c
 * // before accessing the bus, we need to acquire it
 * i2c_acquire(dev);
 * // next we write the register address, but create no STOP condition when done
 * i2c_write_byte(dev, device_addr, reg_addr, (I2C_NOSTOP | I2C_ADDR10));
 * // and now we read the register value
 * i2c_read_byte(dev, device_addr, &reg_value, I2C_ADDR10);
 * // finally we have to release the bus
 * i2c_release(dev);
 * ```
 *
 * Example for writing a 16-bit register with 16-bit register addressing and
 * 7-bit device addressing:
 *
 * ```c
 * // first, acquire the shared bus again
 * i2c_acquire(dev);
 * // write the 16-bit register address to the device and prevent STOP condition
 * i2c_write_byte(dev, device_addr, reg_addr, I2C_NOSTOP);
 * // and write the data after a REPEATED START
 * i2c_write_bytes(dev, device_addr, reg_data, 2, 0);
 * // and finally free the bus again
 * i2c_release(dev);
 * ```
 *
 *
 * @section   sec_i2c_pull Pull Resistors
 *
 * The I2C signal lines SDA/SCL need external pull-up resistors which connect
 * the lines to the positive voltage supply Vcc. The I2C driver implementation
 * should enable the pin's internal pull-up resistors. There are however some
 * use cases for which the internal pull resistors are not strong enough and the
 * I2C bus will show faulty behavior. This can for example happen when
 * connecting a logic analyzer which will raise the capacitance of the bus. In
 * this case you should make sure you connect external pull-up resistors to both
 * I2C bus lines.
 *
 * The minimum and maximum resistances are computed by:
 * @image html periph_i2c_bus_equations.svg
 * <br>The pull-up resistors depend on the bus speed.
 * Some typical values are:<br>
 * Normal mode:       10k&Omega;<br>
 * Fast mode:          2k&Omega;<br>
 * Fast plus mode:     2k&Omega;
 *
 * For more details refer to section 7.1 in:<br>
 * http://www.nxp.com/documents/user_manual/UM10204.pdf
 *
 *
 * @section   sec_i2c_pm (Low-) power implications
 *
 * The I2C interface realizes a transaction-based access scheme to the bus. From
 * a power management perspective, we can leverage this by only powering on the
 * I2C peripheral while it is actually used, that is inside an i2c_acquire() -
 * i2c_release() block.
 *
 * After initialization, the I2C peripheral **should** be powered off (e.g.
 * through peripheral clock gating). It should only be powered on once a
 * transaction on the I2C bus starts, namely in the i2c_acquire() function. Once
 * the transaction is finished, the corresponding I2C peripheral **should** be
 * powered off again in the i2c_release() function.
 *
 * If the implementation puts the active thread to sleep while a transfer is in
 * progress (e.g. when using DMA), the implementation might need to block
 * certain power states.
 *
 * @{
 * @file
 * @brief       Low-level I2C peripheral driver interface definition
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Thomas Eichinger <thomas.eichinger@fu-berlin.de>
 */

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include "periph_conf.h"
#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Default I2C device access macro
 * @{
 */
#ifndef I2C_DEV
#define I2C_DEV(x)          (x)
#endif
/** @} */

/**
 * @brief   Default I2C undefined value
 * @{
 */
#ifndef I2C_UNDEF
#define I2C_UNDEF           (UINT_FAST8_MAX)
#endif
/** @} */

/**
 * @brief   Default i2c_t type definition
 * @{
 */
#ifndef HAVE_I2C_T
typedef uint_fast8_t i2c_t;
#endif
/**  @} */

/**
 * @brief   Read bit needs to be set when reading
 */
#define I2C_READ            (0x0001)

/**
 * @brief   Special bit pattern indicating a 10 bit address is used
 *
 * Should only be used internally in CPU driver implementations, this is not
 * intended to be used by applications.
 *
 * @see https://www.i2c-bus.org/addressing/10-bit-addressing/
 */
#define I2C_10BIT_MAGIC     (0xF0u)

/**
 * @brief   Default mapping of I2C bus speed values
 * @{
 */
#ifndef HAVE_I2C_SPEED_T
typedef enum {
    I2C_SPEED_LOW = 0,      /**< low speed mode:     ~10 kbit/s */
    I2C_SPEED_NORMAL,       /**< normal mode:       ~100 kbit/s */
    I2C_SPEED_FAST,         /**< fast mode:         ~400 kbit/s */
    I2C_SPEED_FAST_PLUS,    /**< fast plus mode:   ~1000 kbit/s */
    I2C_SPEED_HIGH,         /**< high speed mode:  ~3400 kbit/s */
} i2c_speed_t;
#endif
/** @} */

/**
 * @brief   I2C transfer flags
 * @{
 */
#ifndef HAVE_I2C_FLAGS_T
typedef enum {
    I2C_ADDR10  = 0x01,     /**< use 10-bit device addressing */
    I2C_REG16   = 0x02,     /**< use 16-bit register addressing, big-endian */
    I2C_NOSTOP  = 0x04,     /**< do not issue a STOP condition after transfer */
    I2C_NOSTART = 0x08,     /**< skip START sequence, ignores address field */
} i2c_flags_t;
#endif
/** @} */

/**
 * @brief   Initialize the given I2C bus
 *
 * The given I2C device will be initialized with the parameters as specified in
 * the boards periph_conf.h, using the pins and the speed value given there.
 *
 * The bus MUST not be acquired before initializing it, as this is handled
 * internally by the i2c_init function!
 *
 * @warning     This function **MUST NOT** be called by the user unless you add
 *              `DISABLE_MODULE += periph_init_i2c` to your `Makefile`. If you
 *              do so, call this function before any call to `i2c_acquire()`,
 *              and call no more than **once**.
 *
 * @param[in] dev       the device to initialize
 */
void i2c_init(i2c_t dev);

/**
 * @brief   Initialize the used I2C bus pins
 *
 * After calling i2c_init, the pins must be initialized (i.e. spi_init is
 * calling this function internally). In normal cases, this function will not be
 * used. But there are some devices (e.g. ATECC608A), that use I2C bus lines also
 * for other purposes and need the option to dynamically re-configure one or
 * more of the used pins.
 *
 * @param[in] dev       I2C device the pins are configure for
 */
void i2c_init_pins(i2c_t dev);

/**
 * @brief   Change the pins of the given I2C bus back to plain GPIO functionality
 *
 * The pin mux of the SDA and SCL pins of the bus will be changed back to
 * default (GPIO) mode and the I2C bus is powered off.
 * This allows to use the I2C pins for another function and return to I2C
 * functionality again by calling i2c_init_pins()
 *
 * If you want the pin to be in a defined state, call gpio_init() on it.
 *
 * The bus MUST not be acquired before initializing it, as this is handled
 * internally by the i2c_deinit function!
 *
 * Calls to i2c_acquire() will block until i2c_init_pins() is called.
 *
 * @note Until this is implemented on all platforms, this requires the
 *       periph_i2c_reconfigure feature to be used.
 *
 * @param[in] dev       the device to de-initialize
 */
void i2c_deinit_pins(i2c_t dev);

#if DOXYGEN /* functions to be implemented as `#define` in `periph_cpu.h` */
/**
 * @brief   Get the SDA pin of the given I2C bus.
 *
 * @param[in] dev       The device to query
 *
 * @note Until this is implemented on all platforms, this requires the
 *       periph_i2c_reconfigure feature to be used.
 *
 * @return              The GPIO used for the I2C data line.
 */
gpio_t i2c_pin_sda(i2c_t dev);

/**
 * @brief   Get the SCL pin of the given I2C bus.
 *
 * @param[in] dev       The device to query
 *
 * @note Until this is implemented on all platforms, this requires the
 *       periph_i2c_reconfigure feature to be used.
 *
 * @return              The GPIO used for the I2C clock line.
 */
gpio_t i2c_pin_scl(i2c_t dev);
#endif /* DOXYGEN */

/**
 * @brief   Get mutually exclusive access to the given I2C bus
 *
 * In case the I2C device is busy, this function will block until the bus is
 * free again.
 *
 * @pre     Given device is valid, otherwise an assertion blows up
 *          (if assertions are enabled).
 *
 * @param[in] dev           I2C device to access
 */
void i2c_acquire(i2c_t dev);

/**
 * @brief   Release the given I2C device to be used by others
 *
 * @param[in] dev           I2C device to release
 */
void i2c_release(i2c_t dev);

/**
 * @brief   Convenience function for reading one byte from a given register
 *          address
 *
 * @note    This function is using a repeated start sequence for reading from
 *          the specified register address.
 *
 * @pre     i2c_acquire must be called before accessing the bus
 *
 * @param[in]  dev          I2C peripheral device
 * @param[in]  reg          register address to read from (8- or 16-bit,
 *                          right-aligned)
 * @param[in]  addr         7-bit or 10-bit device address (right-aligned)
 * @param[out] data         memory location to store received data
 * @param[in]  flags        optional flags (see @ref i2c_flags_t)
 *
 * @return                  0 When success
 * @return                  -EIO When slave device doesn't ACK the byte
 * @return                  -ENXIO When no devices respond on the address sent on the bus
 * @return                  -ETIMEDOUT  When timeout occurs before device's response
 * @return                  -EINVAL When an invalid argument is given
 * @return                  -EOPNOTSUPP When MCU driver doesn't support the flag operation
 * @return                  -EAGAIN When a lost bus arbitration occurs
 */

int i2c_read_reg(i2c_t dev, uint16_t addr, uint16_t reg,
                 void *data, uint8_t flags);

/**
 * @brief   Convenience function for reading several bytes from a given
 *          register address
 *
 * @note    This function is using a repeated start sequence for reading from
 *          the specified register address.
 *
 * @pre     i2c_acquire must be called before accessing the bus
 *
 * @param[in]  dev          I2C peripheral device
 * @param[in]  reg          register address to read from (8- or 16-bit,
 *                          right-aligned)
 * @param[in]  addr         7-bit or 10-bit device address (right-aligned)
 * @param[out] data         memory location to store received data
 * @param[in]  len          the number of bytes to read into @p data
 * @param[in]  flags        optional flags (see @ref i2c_flags_t)
 *
 * @return                  0 When success
 * @return                  -EIO When slave device doesn't ACK the byte
 * @return                  -ENXIO When no devices respond on the address sent on the bus
 * @return                  -ETIMEDOUT  When timeout occurs before device's response
 * @return                  -EINVAL When an invalid argument is given
 * @return                  -EOPNOTSUPP When MCU driver doesn't support the flag operation
 * @return                  -EAGAIN When a lost bus arbitration occurs
 */
int i2c_read_regs(i2c_t dev, uint16_t addr, uint16_t reg,
                  void *data, size_t len, uint8_t flags);

/**
 * @brief   Convenience function for reading one byte from a device
 *
 * @note    This function is using a repeated start sequence for reading from
 *          the specified register address.
 *
 * @pre     i2c_acquire must be called before accessing the bus
 *
 * @param[in]  dev          I2C peripheral device
 * @param[in]  addr         7-bit or 10-bit device address (right-aligned)
 * @param[out] data         memory location to store received data
 * @param[in]  flags        optional flags (see @ref i2c_flags_t)
 *
 * @return                  0 When success
 * @return                  -EIO When slave device doesn't ACK the byte
 * @return                  -ENXIO When no devices respond on the address sent on the bus
 * @return                  -ETIMEDOUT  When timeout occurs before device's response
 * @return                  -EINVAL When an invalid argument is given
 * @return                  -EOPNOTSUPP When MCU driver doesn't support the flag operation
 * @return                  -EAGAIN When a lost bus arbitration occurs
 */

int i2c_read_byte(i2c_t dev, uint16_t addr, void *data, uint8_t flags);

/**
 * @brief   Convenience function for reading bytes from a device
 *
 * @note    This function is using a repeated start sequence for reading from
 *          the specified register address.
 *
 * @pre     i2c_acquire must be called before accessing the bus
 *
 * @param[in]  dev          I2C peripheral device
 * @param[in]  addr         7-bit or 10-bit device address (right-aligned)
 * @param[out] data         memory location to store received data
 * @param[in]  len          the number of bytes to read into @p data
 * @param[in]  flags        optional flags (see @ref i2c_flags_t)
 *
 * @return                  0 When success
 * @return                  -EIO When slave device doesn't ACK the byte
 * @return                  -ENXIO When no devices respond on the address sent on the bus
 * @return                  -ETIMEDOUT  When timeout occurs before device's response
 * @return                  -EINVAL When an invalid argument is given
 * @return                  -EOPNOTSUPP When MCU driver doesn't support the flag operation
 * @return                  -EAGAIN When a lost bus arbitration occurs
 */

int i2c_read_bytes(i2c_t dev, uint16_t addr,
                   void *data, size_t len, uint8_t flags);

/**
 * @brief   Convenience function for writing a single byte onto the bus
 *
 * @pre     i2c_acquire must be called before accessing the bus
 *
 * @param[in] dev           I2C peripheral device
 * @param[in] addr          7-bit or 10-bit device address (right-aligned)
 * @param[in] data          byte to write to the device
 * @param[in] flags         optional flags (see @ref i2c_flags_t)
 *
 * @return                  0 When success
 * @return                  -EIO When slave device doesn't ACK the byte
 * @return                  -ENXIO When no devices respond on the address sent on the bus
 * @return                  -ETIMEDOUT  When timeout occurs before device's response
 * @return                  -EINVAL When an invalid argument is given
 * @return                  -EOPNOTSUPP When MCU driver doesn't support the flag operation
 * @return                  -EAGAIN When a lost bus arbitration occurs
 */
int i2c_write_byte(i2c_t dev, uint16_t addr, uint8_t data, uint8_t flags);

/**
 * @brief   Convenience function for writing several bytes onto the bus
 *
 * @pre     i2c_acquire must be called before accessing the bus
 *
 * @param[in] dev           I2C peripheral device
 * @param[in] addr          7-bit or 10-bit device address (right-aligned)
 * @param[in] data          array holding the bytes to write to the device
 * @param[in] len           the number of bytes to write
 * @param[in] flags         optional flags (see @ref i2c_flags_t)
 *
 * @return                  0 When success
 * @return                  -EIO When slave device doesn't ACK the byte
 * @return                  -ENXIO When no devices respond on the address sent on the bus
 * @return                  -ETIMEDOUT  When timeout occurs before device's response
 * @return                  -EINVAL When an invalid argument is given
 * @return                  -EOPNOTSUPP When MCU driver doesn't support the flag operation
 * @return                  -EAGAIN When a lost bus arbitration occurs
 */
int i2c_write_bytes(i2c_t dev, uint16_t addr, const void *data,
                    size_t len, uint8_t flags);

/**
 * @brief   Convenience function for writing one byte to a given
 *          register address
 *
 * @note    This function is using a continuous sequence for writing to the
 *          specified register address. It first writes the register then data.
 *
 * @pre     i2c_acquire must be called before accessing the bus
 *
 * @param[in]  dev          I2C peripheral device
 * @param[in]  reg          register address to read from (8- or 16-bit,
 *                          right-aligned)
 * @param[in]  addr         7-bit or 10-bit device address (right-aligned)
 * @param[in]  data         byte to write
 * @param[in]  flags        optional flags (see @ref i2c_flags_t)
 *
 * @return                  0 When success
 * @return                  -EIO When slave device doesn't ACK the byte
 * @return                  -ENXIO When no devices respond on the address sent on the bus
 * @return                  -ETIMEDOUT  When timeout occurs before device's response
 * @return                  -EINVAL When an invalid argument is given
 * @return                  -EOPNOTSUPP When MCU driver doesn't support the flag operation
 * @return                  -EAGAIN When a lost bus arbitration occurs
 */
int i2c_write_reg(i2c_t dev, uint16_t addr, uint16_t reg,
                  uint8_t data, uint8_t flags);

/**
 * @brief   Convenience function for writing data to a given register address
 *
 * @note    This function is using a continuous sequence for writing to the
 *          specified register address. It first writes the register then data.
 *
 * @pre     i2c_acquire must be called before accessing the bus
 *
 * @param[in]  dev          I2C peripheral device
 * @param[in]  reg          register address to read from (8- or 16-bit,
 *                          right-aligned)
 * @param[in]  addr         7-bit or 10-bit device address (right-aligned)
 * @param[out] data         memory location to store received data
 * @param[in]  len          the number of bytes to write
 * @param[in]  flags        optional flags (see @ref i2c_flags_t)
 *
 * @return                  0 When success
 * @return                  -EIO When slave device doesn't ACK the byte
 * @return                  -ENXIO When no devices respond on the address sent on the bus
 * @return                  -ETIMEDOUT  When timeout occurs before device's response
 * @return                  -EINVAL When an invalid argument is given
 * @return                  -EOPNOTSUPP When MCU driver doesn't support the flag operation
 * @return                  -EAGAIN When a lost bus arbitration occurs
 */
int i2c_write_regs(i2c_t dev, uint16_t addr, uint16_t reg,
                  const void *data, size_t len, uint8_t flags);

#ifdef __cplusplus
}
#endif

/** @} */
```

## Source: `data/rtos/riot/sys/include/ztimer.h`

```c

/* excerpt lines 1-60 */
/*
 * Copyright (C) 2018 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

/**
 * @defgroup    sys_ztimer ztimer high level timer abstraction layer
 * @ingroup     sys
 * @brief       High level timer abstraction layer
 *
 * # Introduction
 *
 * ztimer provides a high level abstraction of hardware timers for application
 * timing needs.
 *
 * The basic functions of the ztimer module are ztimer_now(), ztimer_sleep(),
 * ztimer_set() and ztimer_remove().
 *
 * They all take a pointer to a clock device (or virtual timer device) as first
 * parameter.
 *
 * RIOT provides ZTIMER_USEC, ZTIMER_MSEC, ZTIMER_SEC by default, which can be
 * used in an application by depending on the modules ztimer_usec, ztimer_msec
 * or ztimer_sec. They will then automatically get configured.
 *
 * Every ztimer clock allows multiple timeouts to be scheduled. They all
 * provide unsigned 32bit range. In this documentation, a timeout or its
 * corresponding struct will be called `timer`, and when the time out has
 * passed, it has `triggered`.
 *
 * As ztimer can use arbitrarily configurable backends, a ztimer clock instance
 * can run at configurable frequencies. Throughout this documentation, one
 * clock step is called `tick`.  For the pre-defined clocks ZTIMER_USEC,
 * ZTIMER_MSEC and ZTIMER_SEC, one clock tick corresponds to one microsecond,
 * one millisecond or one second, respectively.
 *
 * ztimer_now() returns the current clock tick count as uint32_t.
 *
 * ztimer_sleep() pauses the current thread for the passed amount of clock
 * ticks. E.g., ```ztimer_sleep(ZTIMER_SEC, 5);``` will suspend the currently
 * running thread for five seconds.
 *
 * ztimer_set() takes a ztimer_t object (containing a function pointer and
 * void * argument) and an interval as arguments. After at least the interval
 * (in number of ticks for the corresponding clock) has passed, the callback
 * will be called in interrupt context.
 * A timer can be cancelled using ztimer_remove().
 *
 * Example:
 *
 * ```
 * #include "ztimer.h"
 *
 * static void callback(void *arg)
 * {

/* ... additional content omitted by deterministic excerpt limit ... */
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
