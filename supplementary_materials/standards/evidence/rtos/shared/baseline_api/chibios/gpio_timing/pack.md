# Raw RTOS/Bus Pack: chibios / gpio_timing

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/chibios/os/hal/include/hal.h`

```c
/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    hal.h
 * @brief   HAL subsystem header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef HAL_H
#define HAL_H

#include "ccportab.h"

#include "osal.h"
#include "board.h"
#include "halconf.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   ChibiOS/HAL identification macro.
 */
#define __CHIBIOS_HAL__

/**
 * @brief   Stable release flag.
 */
#define CH_HAL_STABLE           0

/**
 * @name    ChibiOS/HAL version identification
 * @{
 */
/**
 * @brief   HAL version string.
 */
#define CH_HAL_VERSION          "9.1.0"

/**
 * @brief   HAL version major number.
 */
#define CH_HAL_MAJOR            9

/**
 * @brief   HAL version minor number.
 */
#define CH_HAL_MINOR            1

/**
 * @brief   HAL version patch number.
 */
#define CH_HAL_PATCH            0
/** @} */

/**
 * @name    Return codes
 * @{
 */
/**
 * @brief HAL operation success.
 * @deprecated
 */
#define HAL_SUCCESS             false
/**
 * @brief HAL operation failed.
 * @deprecated
 */
#define HAL_FAILED              true
/** @} */

/**
 * @name    Return codes for HAL functions
 * @{
 */
#define HAL_RET_SUCCESS         MSG_OK
/**
 * @brief   Configuration error.
 * @details An error has been detected in the driver configuration structure.
 */
#define HAL_RET_CONFIG_ERROR    (msg_t)-16
/**
 * @brief   A required resource is not available.
 * @details One of the resources required for driver operations is not
 *          available.
 */
#define HAL_RET_NO_RESOURCE     (msg_t)-17
/**
 * @brief   The peripheral is busy.
 * @details The peripheral is not available or taken by some other system
 *          actor.
 */
#define HAL_RET_HW_BUSY         (msg_t)-18
/**
 * @brief   Peripheral failure.
 * @details Peripheral failed, for example HW timeouts.
 */
#define HAL_RET_HW_FAILURE      (msg_t)-19
/**
 * @brief   Unknown control code.
 */
#define HAL_RET_UNKNOWN_CTL     (msg_t)-20
/**
 * @brief   Invalid instance pointer.
 */
#define HAL_RET_IS_INVALID      (msg_t)-21
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Configuration file checks.*/
#if !defined(_CHIBIOS_HAL_CONF_)
#error "invalid configuration file"
#endif

#if !defined(_CHIBIOS_HAL_CONF_VER_9_1_)
#error "obsolete or unknown configuration file"
#endif

/* Error checks on the configuration header file.*/
#if !defined(HAL_USE_PAL)
#define HAL_USE_PAL                         FALSE
#endif

#if !defined(HAL_USE_ADC)
#define HAL_USE_ADC                         FALSE
#endif

#if !defined(HAL_USE_CAN)
#define HAL_USE_CAN                         FALSE
#endif

#if !defined(HAL_USE_CRY)
#define HAL_USE_CRY                         FALSE
#endif

#if !defined(HAL_USE_DAC)
#define HAL_USE_DAC                         FALSE
#endif

#if !defined(HAL_USE_EFL)
#define HAL_USE_EFL                         FALSE
#endif

#if !defined(HAL_USE_GPT)
#define HAL_USE_GPT                         FALSE
#endif

#if !defined(HAL_USE_I2C)
#define HAL_USE_I2C                         FALSE
#endif

#if !defined(HAL_USE_I2S)
#define HAL_USE_I2S                         FALSE
#endif

#if !defined(HAL_USE_ICU)
#define HAL_USE_ICU                         FALSE
#endif

#if !defined(HAL_USE_MAC)
#define HAL_USE_MAC                         FALSE
#endif

#if !defined(HAL_USE_PWM)
#define HAL_USE_PWM                         FALSE
#endif

#if !defined(HAL_USE_RTC)
#define HAL_USE_RTC                         FALSE
#endif

#if !defined(HAL_USE_SERIAL)
#define HAL_USE_SERIAL                      FALSE
#endif

#if !defined(HAL_USE_SDC)
#define HAL_USE_SDC                         FALSE
#endif

#if !defined(HAL_USE_SIO)
#define HAL_USE_SIO                         FALSE
#endif

#if !defined(HAL_USE_SPI)
#define HAL_USE_SPI                         FALSE
#endif

#if !defined(HAL_USE_TRNG)
#define HAL_USE_TRNG                        FALSE
#endif

#if !defined(HAL_USE_UART)
#define HAL_USE_UART                        FALSE
#endif

#if !defined(HAL_USE_USB)
#define HAL_USE_USB                         FALSE
#endif

#if !defined(HAL_USE_WDG)
#define HAL_USE_WDG                         FALSE
#endif

#if !defined(HAL_USE_WSPI)
#define HAL_USE_WSPI                        FALSE
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a clock point identifier.
 */
typedef unsigned halclkpt_t;

/**
 * @brief   Type of a clock point frequency in Hz.
 */
typedef uint32_t halfreq_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void halInit(void);
#ifdef __cplusplus
}
#endif

/* Low Level HAL support.*/
#include "hal_lld.h"

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Returns the frequency of a clock point in Hz.
 *
 * @param[in] clkpt     clock point to be returned
 * @return              The clock point frequency in Hz or zero if the
 *                      frequency is unknown.
 *
 * @xclass
 */
static inline halfreq_t halClockGetPointX(halclkpt_t clkpt) {

  (void)clkpt;  /* LLD macro could not use it.*/

  return hal_lld_get_clock_point(clkpt);
}

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   Switches to a different clock configuration
 *
 * @param[in] ccp       pointer to a @p halclkcfg_t structure
 * @return              The clock switch result.
 * @retval false        if the clock switch succeeded
 * @retval true         if the clock switch failed
 *
 * @special
 */
static inline bool halClockSwitchMode(const halclkcfg_t *ccp) {

  return hal_lld_clock_switch_mode(ccp);
}
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/*===========================================================================*/
/* Driver late inclusions.                                                   */
/*===========================================================================*/

/* Abstract interfaces.*/
#include "hal_objects.h"
#include "hal_streams.h"
#include "hal_channels.h"
#include "hal_files.h"
#include "hal_ioblock.h"
#include "hal_mmcsd.h"
#include "hal_persistent.h"
#include "hal_flash.h"

/* Shared headers.*/
#include "hal_safety.h"
#include "hal_buffers.h"
#include "hal_queues.h"
#include "hal_buffered_serial.h"

/* Normal drivers.*/
#include "hal_pal.h"
#include "hal_adc.h"
#include "hal_can.h"
#include "hal_crypto.h"
#include "hal_dac.h"
#include "hal_efl.h"
#include "hal_gpt.h"
#include "hal_i2c.h"
#include "hal_i2s.h"
#include "hal_icu.h"
#include "hal_mac.h"
#include "hal_pwm.h"
#include "hal_rtc.h"
#include "hal_serial.h"
#include "hal_sdc.h"
#include "hal_sio.h"
#include "hal_spi.h"
#include "hal_trng.h"
#include "hal_uart.h"
#include "hal_usb.h"
#include "hal_wdg.h"
#include "hal_wspi.h"

/*
 *  The ST driver is a special case, it is only included if the OSAL is
 *  configured to require it.
 */
#if OSAL_ST_MODE != OSAL_ST_MODE_NONE
#include "hal_st.h"
#endif

/* Complex drivers.*/
#include "hal_mmc_spi.h"
#include "hal_serial_usb.h"

/* Community drivers.*/
#if defined(HAL_USE_COMMUNITY) || defined(__DOXYGEN__)
#if (HAL_USE_COMMUNITY == TRUE) || defined(__DOXYGEN__)
#include "hal_community.h"
#endif
#endif

#endif /* HAL_H */

/** @} */
```

## Source: `data/rtos/chibios/os/hal/include/hal_pal.h`

```c
/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    hal_pal.h
 * @brief   I/O Ports Abstraction Layer macros, types and structures.
 *
 * @addtogroup PAL
 * @{
 */

#ifndef HAL_PAL_H
#define HAL_PAL_H

#if (HAL_USE_PAL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Pads mode constants
 * @{
 */
/**
 * @brief   After reset state.
 * @details The state itself is not specified and is architecture dependent,
 *          it is guaranteed to be equal to the after-reset state. It is
 *          usually an input state.
 */
#define PAL_MODE_RESET                  0U

/**
 * @brief   Safe state for <b>unconnected</b> pads.
 * @details The state itself is not specified and is architecture dependent,
 *          it may be mapped on @p PAL_MODE_INPUT_PULLUP,
 *          @p PAL_MODE_INPUT_PULLDOWN or @p PAL_MODE_OUTPUT_PUSHPULL for
 *          example.
 */
#define PAL_MODE_UNCONNECTED            1U

/**
 * @brief   Regular input high-Z pad.
 */
#define PAL_MODE_INPUT                  2U

/**
 * @brief   Input pad with weak pull up resistor.
 */
#define PAL_MODE_INPUT_PULLUP           3U

/**
 * @brief   Input pad with weak pull down resistor.
 */
#define PAL_MODE_INPUT_PULLDOWN         4U

/**
 * @brief   Analog input mode.
 */
#define PAL_MODE_INPUT_ANALOG           5U

/**
 * @brief   Push-pull output pad.
 */
#define PAL_MODE_OUTPUT_PUSHPULL        6U

/**
 * @brief   Open-drain output pad.
 */
#define PAL_MODE_OUTPUT_OPENDRAIN       7U
/** @} */

/**
 * @name    Logic level constants
 * @{
 */
/**
 * @brief   Logical low state.
 */
#define PAL_LOW                         0U

/**
 * @brief   Logical high state.
 */
#define PAL_HIGH                        1U
/** @} */

/**
 * @name    PAL event modes
 * @{
 */
#define PAL_EVENT_MODE_EDGES_MASK   3U  /**< @brief Mask of edges field.    */
#define PAL_EVENT_MODE_DISABLED     0U  /**< @brief Channel disabled.       */
#define PAL_EVENT_MODE_RISING_EDGE  1U  /**< @brief Rising edge callback.   */
#define PAL_EVENT_MODE_FALLING_EDGE 2U  /**< @brief Falling edge callback.  */
#define PAL_EVENT_MODE_BOTH_EDGES   3U  /**< @brief Both edges callback.    */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    PAL configuration options
 * @{
 */
/**
 * @brief   Enables synchronous APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(PAL_USE_CALLBACKS) || defined(__DOXYGEN__)
#define PAL_USE_CALLBACKS           TRUE
#endif

/**
 * @brief   Enables synchronous APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(PAL_USE_WAIT) || defined(__DOXYGEN__)
#define PAL_USE_WAIT                TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a PAL event callback.
 */
typedef void (*palcallback_t)(void *arg);

#if (PAL_USE_WAIT == TRUE) || (PAL_USE_CALLBACKS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Type of a PAL event record.
 */
typedef struct {
#if (PAL_USE_WAIT == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   Threads queued for an event.
   */
  threads_queue_t       threads;
#endif
#if (PAL_USE_CALLBACKS == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   Event callback.
   */
  palcallback_t         cb;
  /**
   * @brief   Event callback argument.
   */
  void                  *arg;
#endif
} palevent_t;
#endif

#include "hal_pal_lld.h"

/**
 * @brief   I/O bus descriptor.
 * @details This structure describes a group of contiguous digital I/O lines
 *          that have to be handled as bus.
 * @note    I/O operations on a bus do not affect I/O lines on the same port but
 *          not belonging to the bus.
 */
typedef struct {
  /**
   * @brief Port identifier.
   */
  ioportid_t            portid;
  /**
   * @brief Bus mask aligned to port bit 0.
   * @note  The bus mask implicitly define the bus width. A logic AND is
   *        performed on the bus data.
   */
  ioportmask_t          mask;
  /**
   * @brief Offset, within the port, of the least significant bit of the bus.
   */
  uint_fast8_t          offset;
} IOBus;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Port bit helper macro.
 * @details This macro calculates the mask of a bit within a port.
 *
 * @param[in] n         bit position within the port
 * @return              The bit mask.
 */
#if !defined(PAL_PORT_BIT) || defined(__DOXYGEN__)
#define PAL_PORT_BIT(n) ((ioportmask_t)(1U << (n)))
#endif

/**
 * @brief   Bits group mask helper.
 * @details This macro calculates the mask of a bits group.
 *
 * @param[in] width     group width
 * @return              The group mask.
 */
#if !defined(PAL_GROUP_MASK) || defined(__DOXYGEN__)
#define PAL_GROUP_MASK(width) ((ioportmask_t)(1U << (width)) - 1U)
#endif

/**
 * @brief   Data part of a static I/O bus initializer.
 * @details This macro should be used when statically initializing an I/O bus
 *          that is part of a bigger structure.
 *
 * @param[in] name      name of the IOBus variable
 * @param[in] port      I/O port descriptor
 * @param[in] width     bus width in bits
 * @param[in] offset    bus bit offset within the port
 */
#define _IOBUS_DATA(name, port, width, offset)                              \
  {port, PAL_GROUP_MASK(width), offset}

/**
 * @brief   Static I/O bus initializer.
 *
 * @param[in] name      name of the IOBus variable
 * @param[in] port      I/O port descriptor
 * @param[in] width     bus width in bits
 * @param[in] offset    bus bit offset within the port
 */
#define IOBUS_DECL(name, port, width, offset)                               \
  IOBus name = _IOBUS_DATA(name, port, width, offset)

#if (PAL_USE_CALLBACKS == TRUE) || (PAL_USE_WAIT == TRUE) ||                \
    defined(__DOXYGEN__)
/**
 * @name    Low level driver helper macros
 * @{
 */
#if ((PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == TRUE)) ||              \
    defined(__DOXYGEN__)
/**
 * @brief   Initializes a PAL event object.
 *
 * @param[in] e         event index
 *
 * @notapi
 */
#define _pal_init_event(e)                                                  \
  do {                                                                      \
    osalThreadQueueObjectInit(&_pal_events[e].threads);                     \
    _pal_events[e].cb = NULL;                                               \
    _pal_events[e].arg = NULL;                                              \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == TRUE) */

#if (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == FALSE)
#define _pal_init_event(e)                                                  \
  do {                                                                      \
    _pal_events[e].cb = NULL;                                               \
    _pal_events[e].arg = NULL;                                              \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == FALSE) */

#if (PAL_USE_CALLBACKS == FALSE) && (PAL_USE_WAIT == TRUE)
#define _pal_init_event(e)                                                  \
  do {                                                                      \
    osalThreadQueueObjectInit(&_pal_events[e].threads);                     \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == FALSE) && (PAL_USE_WAIT == TRUE) */

#if ((PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == TRUE)) || defined(__DOXYGEN__)
/**
 * @brief   Clears a PAL event object.
 *
 * @param[in] e         event index
 *
 * @notapi
 */
#define _pal_clear_event(e)                                                 \
  do {                                                                      \
    osalThreadDequeueAllI(&_pal_events[e].threads, MSG_RESET);              \
    _pal_events[e].cb = NULL;                                               \
    _pal_events[e].arg = NULL;                                              \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == TRUE) */

#if (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == FALSE)
#define _pal_clear_event(e)                                                 \
  do {                                                                      \
    _pal_events[e].cb = NULL;                                               \
    _pal_events[e].arg = NULL;                                              \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == FALSE) */

#if (PAL_USE_CALLBACKS == FALSE) && (PAL_USE_WAIT == TRUE)
#define _pal_clear_event(e)                                                 \
  do {                                                                      \
    osalThreadDequeueAllI(&_pal_events[e].threads, MSG_RESET);              \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == FALSE) && (PAL_USE_WAIT == TRUE) */

/**
 * @brief   Common ISR code.
 * @note    This macro is meant to be used in the low level drivers
 *          implementation only.
 *
 * @param[in] e         event index
 *
 * @notapi
 */
#if ((PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == TRUE)) ||              \
    defined(__DOXYGEN__)
#define _pal_isr_code(e)                                                    \
  do {                                                                      \
    if (_pal_events[e].cb != NULL) {                                        \
      _pal_events[e].cb(_pal_events[e].arg);                                \
    }                                                                       \
    osalSysLockFromISR();                                                   \
    osalThreadDequeueAllI(&_pal_events[e].threads, MSG_OK);                 \
    osalSysUnlockFromISR();                                                 \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == TRUE) */

#if (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == FALSE)
#define _pal_isr_code(e)                                                    \
  do {                                                                      \
    if (_pal_events[e].cb != NULL) {                                        \
      _pal_events[e].cb(_pal_events[e].arg);                                \
    }                                                                       \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == TRUE) && (PAL_USE_WAIT == FALSE) */

#if ((PAL_USE_CALLBACKS == FALSE) && (PAL_USE_WAIT == TRUE)) ||             \
    defined(__DOXYGEN__)
#define _pal_isr_code(e)                                                    \
  do {                                                                      \
    osalSysLockFromISR();                                                   \
    osalThreadDequeueAllI(&_pal_events[e].threads, MSG_OK);                 \
    osalSysUnlockFromISR();                                                 \
  } while (false)
#endif /* (PAL_USE_CALLBACKS == FALSE) && (PAL_USE_WAIT == TRUE) */

/** @} */
#endif /* (PAL_USE_CALLBACKS == TRUE) || (PAL_USE_WAIT == TRUE) */

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   PAL subsystem initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
#if defined(PAL_NEW_INIT) || defined(__DOXYGEN__)
#define palInit() pal_lld_init()
#else
#define palInit(config) pal_lld_init(config)
#endif

/**
 * @brief   Reads the physical I/O port states.
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @return              The port logic states.
 *
 * @special
 */
#if !defined(pal_lld_readport) || defined(__DOXYGEN__)
#define palReadPort(port) ((void)(port), 0U)
#else
#define palReadPort(port) pal_lld_readport(port)
#endif

/**
 * @brief   Reads the output latch.
 * @details The purpose of this function is to read back the latched output
 *          value.
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @return              The latched logic states.
 *
 * @special
 */
#if !defined(pal_lld_readlatch) || defined(__DOXYGEN__)
#define palReadLatch(port) ((void)(port), 0U)
#else
#define palReadLatch(port) pal_lld_readlatch(port)
#endif

/**
 * @brief   Writes a bits mask on a I/O port.
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be written on the specified port
 *
 * @special
 */
#if !defined(pal_lld_writeport) || defined(__DOXYGEN__)
#define palWritePort(port, bits) ((void)(port), (void)(bits))
#else
#define palWritePort(port, bits) pal_lld_writeport(port, bits)
#endif

/**
 * @brief   Sets a bits mask on a I/O port.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures, for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be ORed on the specified port
 *
 * @special
 */
#if !defined(pal_lld_setport) || defined(__DOXYGEN__)
#define palSetPort(port, bits)                                              \
  palWritePort(port, palReadLatch(port) | (bits))
#else
#define palSetPort(port, bits) pal_lld_setport(port, bits)
#endif

/**
 * @brief   Clears a bits mask on a I/O port.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures,  for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be cleared on the specified port
 *
 * @special
 */
#if !defined(pal_lld_clearport) || defined(__DOXYGEN__)
#define palClearPort(port, bits)                                            \
  palWritePort(port, palReadLatch(port) & ~(bits))
#else
#define palClearPort(port, bits) pal_lld_clearport(port, bits)
#endif

/**
 * @brief   Toggles a bits mask on a I/O port.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures, for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be XORed on the specified port
 *
 * @special
 */
#if !defined(pal_lld_toggleport) || defined(__DOXYGEN__)
#define palTogglePort(port, bits)                                           \
  palWritePort(port, palReadLatch(port) ^ (bits))
#else
#define palTogglePort(port, bits) pal_lld_toggleport(port, bits)
#endif

/**
 * @brief   Reads a group of bits.
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask, a logic AND is performed on the input
 *                      data
 * @param[in] offset    group bit offset within the port
 * @return              The group logic states.
 *
 * @special
 */
#if !defined(pal_lld_readgroup) || defined(__DOXYGEN__)
#define palReadGroup(port, mask, offset)                                    \
  ((palReadPort(port) >> (offset)) & (mask))
#else
#define palReadGroup(port, mask, offset) pal_lld_readgroup(port, mask, offset)
#endif

/**
 * @brief   Reads the group latch.
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask, a logic AND is performed on the input
 *                      data
 * @param[in] offset    group bit offset within the port
 * @return              The group logic states.
 *
 * @special
 */
#if !defined(pal_lld_readgrouplatch) || defined(__DOXYGEN__)
#define palReadGroupLatch(port, mask, offset)                               \
  ((palReadLatch(port) >> (offset)) & (mask))
#else
#define palReadGroupLatch(port, mask, offset) pal_lld_readgrouplatch(port, mask, offset)
#endif

/**
 * @brief   Writes a group of bits.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures, for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask, a logic AND is performed on the
 *                      output  data
 * @param[in] offset    group bit offset within the port
 * @param[in] bits      bits to be written. Values exceeding the group
 *                      width are masked.
 *
 * @special
 */
#if !defined(pal_lld_writegroup) || defined(__DOXYGEN__)
#define palWriteGroup(port, mask, offset, bits)                             \
  palWritePort(port, (palReadLatch(port) & ~((mask) << (offset))) |         \
                     (((bits) & (mask)) << (offset)))
#else
#define palWriteGroup(port, mask, offset, bits)                             \
  pal_lld_writegroup(port, mask, offset, bits)
#endif

/**
 * @brief   Pads group mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures, for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    Programming an unknown or unsupported mode is silently ignored.
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] mask      group mask
 * @param[in] offset    group bit offset within the port
 * @param[in] mode      group mode
 *
 * @special
 */
#if !defined(pal_lld_setgroupmode) || defined(__DOXYGEN__)
#define palSetGroupMode(port, mask, offset, mode)                           \
  do {                                                                      \
    (void)(port);                                                           \
    (void)(mask);                                                           \
    (void)(offset);                                                         \
    (void)(mode);                                                           \
  } while (false)
#else
#define palSetGroupMode(port, mask, offset, mode)                           \
  pal_lld_setgroupmode(port, mask, offset, mode)
#endif

/**
 * @brief   Reads an input pad logic state.
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 * @return              The logic state.
 * @retval PAL_LOW      low logic state.
 * @retval PAL_HIGH     high logic state.
 *
 * @special
 */
#if !defined(pal_lld_readpad) || defined(__DOXYGEN__)
#define palReadPad(port, pad) ((palReadPort(port) >> (pad)) & 1U)
#else
#define palReadPad(port, pad) pal_lld_readpad(port, pad)
#endif

/**
 * @brief   Writes a logic state on an output pad.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures, for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 * @param[in] bit       logic value, the value must be @p PAL_LOW or
 *                      @p PAL_HIGH
 *
 * @special
 */
#if !defined(pal_lld_writepad) || defined(__DOXYGEN__)
#define palWritePad(port, pad, bit)                                         \
  palWritePort(port, (palReadLatch(port) & ~PAL_PORT_BIT(pad)) |            \
                     (((bit) & 1U) << pad))
#else
#define palWritePad(port, pad, bit) pal_lld_writepad(port, pad, bit)
#endif

/**
 * @brief   Sets a pad logic state to @p PAL_HIGH.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures, for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 *
 * @special
 */
#if !defined(pal_lld_setpad) || defined(__DOXYGEN__)
#define palSetPad(port, pad) palSetPort(port, PAL_PORT_BIT(pad))
#else
#define palSetPad(port, pad) pal_lld_setpad(port, pad)
#endif

/**
 * @brief   Clears a pad logic state to @p PAL_LOW.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures, for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 *
 * @special
 */
#if !defined(pal_lld_clearpad) || defined(__DOXYGEN__)
#define palClearPad(port, pad) palClearPort(port, PAL_PORT_BIT(pad))
#else
#define palClearPad(port, pad) pal_lld_clearpad(port, pad)
#endif

/**
 * @brief   Toggles a pad logic state.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures, for atomicity and/or portability reasons you may
 *          need to enclose port I/O operations between @p osalSysLock() and
 *          @p osalSysUnlock().
 * @note    The function can be called from any context.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 *
 * @special
 */
#if !defined(pal_lld_togglepad) || defined(__DOXYGEN__)
#define palTogglePad(port, pad) palTogglePort(port, PAL_PORT_BIT(pad))
#else
#define palTogglePad(port, pad) pal_lld_togglepad(port, pad)
#endif

/**
 * @brief   Pad mode setup.
 * @details This function programs a pad with the specified mode.
 * @note    The operation is not guaranteed to be atomic on all the
 *          architectures,
/* ... truncated ... */
```

## Source: `data/rtos/chibios/os/rt/include/ch.h`

```c
/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    rt/include/ch.h
 * @brief   ChibiOS/RT main include file.
 *
 * @addtogroup kernel_info
 * @details This header includes all the required kernel headers so it is the
 *          only kernel header you usually want to include in your application.
 * @details Kernel related info.
 * @{
 */

#ifndef CH_H
#define CH_H

/**
 * @brief   ChibiOS/RT identification macro.
 */
#define __CHIBIOS_RT__

/**
 * @brief   Stable release flag.
 */
#define CH_KERNEL_STABLE        0

/**
 * @name    ChibiOS/RT version identification
 * @{
 */
/**
 * @brief   Kernel version string.
 */
#define CH_KERNEL_VERSION       "8.0.0"

/**
 * @brief   Kernel version major number.
 */
#define CH_KERNEL_MAJOR         8

/**
 * @brief   Kernel version minor number.
 */
#define CH_KERNEL_MINOR         0

/**
 * @brief   Kernel version patch number.
 */
#define CH_KERNEL_PATCH         0
/** @} */

/**
 * @name    Constants for configuration options
 * @{
 */
/**
 * @brief   Generic 'false' preprocessor boolean constant.
 * @note    It is meant to be used in configuration files as switch.
 */
#if !defined(FALSE) || defined(__DOXYGEN__)
#define FALSE                   0
#endif

/**
 * @brief   Generic 'true' preprocessor boolean constant.
 * @note    It is meant to be used in configuration files as switch.
 */
#if !defined(TRUE) || defined(__DOXYGEN__)
#define TRUE                    1
#endif
/** @} */

/* License.*/
#include "chlicense.h"

/* Configuration headers, checks and licensing restrictions.*/
#include "chconf.h"
#include "chchecks.h"
#include "chrestrictions.h"

/* Base kernel headers.*/
#include "chearly.h"
#include "chrfcu.h"
#include "chdebug.h"
#include "chtime.h"
#include "chalign.h"
#include "chtrace.h"
#include "chport.h"
#include "chsafety.h"
#include "chlists.h"
#include "chtm.h"
#include "chstats.h"
#include "chobjects.h"
#include "chmem.h"
#include "chsys.h"
#include "chinstances.h"
#include "chvt.h"
#include "chschd.h"
#include "chthreads.h"

/* Optional subsystems headers.*/
#include "chregistry.h"
#include "chsem.h"
#include "chmtx.h"
#include "chcond.h"
#include "chevents.h"
#include "chmsg.h"

/* OSLIB.*/
#include "chlib.h"

/* Headers dependent on the OSLIB.*/
#include "chdynamic.h"

#endif /* CH_H */

/** @} */
```

## Source: `data/rtos/chibios/os/rt/include/chtime.h`

```c
/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    rt/include/chtime.h
 * @brief   Time and intervals macros and structures.
 *
 * @addtogroup time_intervals
 * @details This module is responsible for handling of system time and time
 *          intervals.
 * @{
 */

#ifndef CHTIME_H
#define CHTIME_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Special time constants
 * @{
 */
/**
 * @brief   Zero interval specification for some functions with a timeout
 *          specification.
 * @note    Not all functions accept @p TIME_IMMEDIATE as timeout parameter,
 *          see the specific function documentation.
 */
#define TIME_IMMEDIATE      ((sysinterval_t)0)

/**
 * @brief   Infinite interval specification for all functions with a timeout
 *          specification.
 * @note    Not all functions accept @p TIME_INFINITE as timeout parameter,
 *          see the specific function documentation.
 */
#define TIME_INFINITE       ((sysinterval_t)-1)

/**
 * @brief   Maximum interval constant usable as timeout.
 */
#define TIME_MAX_INTERVAL   ((sysinterval_t)-2)

/**
 * @brief   Maximum system of system time before it wraps.
 */
#define TIME_MAX_SYSTIME    ((systime_t)-1)
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (CH_CFG_ST_RESOLUTION != 16) && (CH_CFG_ST_RESOLUTION != 32) &&         \
    (CH_CFG_ST_RESOLUTION != 64)
#error "invalid CH_CFG_ST_RESOLUTION specified, must be 16, 32 or 64"
#endif

#if CH_CFG_ST_FREQUENCY < 10
#error "invalid CH_CFG_ST_FREQUENCY specified, must be >= 10"
#endif

#if (CH_CFG_INTERVALS_SIZE != 16) && (CH_CFG_INTERVALS_SIZE != 32) &&       \
    (CH_CFG_INTERVALS_SIZE != 64)
#error "invalid CH_CFG_INTERVALS_SIZE specified, must be 16, 32 or 64"
#endif

#if (CH_CFG_TIME_TYPES_SIZE != 16) && (CH_CFG_TIME_TYPES_SIZE != 32)
#error "invalid CH_CFG_TIME_TYPES_SIZE specified, must be 16 or 32"
#endif

#if CH_CFG_INTERVALS_SIZE < CH_CFG_ST_RESOLUTION
#error "CH_CFG_INTERVALS_SIZE must be >= CH_CFG_ST_RESOLUTION"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of system time.
 * @note    It is selectable in configuration between 16, 32 or 64 bits.
 */
#if (CH_CFG_ST_RESOLUTION == 64) || defined(__DOXYGEN__)
typedef uint64_t systime_t;
#elif CH_CFG_ST_RESOLUTION == 32
typedef uint32_t systime_t;
#elif CH_CFG_ST_RESOLUTION == 16
typedef uint16_t systime_t;
#endif

/**
 * @brief   Type of time interval.
 * @note    It is selectable in configuration between 16, 32 or 64 bits.
 */
#if (CH_CFG_INTERVALS_SIZE == 64) || defined(__DOXYGEN__)
typedef uint64_t sysinterval_t;
#elif CH_CFG_INTERVALS_SIZE == 32
typedef uint32_t sysinterval_t;
#elif CH_CFG_INTERVALS_SIZE == 16
typedef uint16_t sysinterval_t;
#endif

/**
 * @brief   Type of a time stamp.
 */
typedef uint64_t systimestamp_t;

#if (CH_CFG_TIME_TYPES_SIZE == 32) || defined(__DOXYGEN__)
/**
 * @brief   Type of seconds.
 * @note    It is selectable in configuration between 16 or 32 bits.
 */
typedef uint32_t time_secs_t;

/**
 * @brief   Type of milliseconds.
 * @note    It is selectable in configuration between 16 or 32 bits.
 */
typedef uint32_t time_msecs_t;

/**
 * @brief   Type of microseconds.
 * @note    It is selectable in configuration between 16 or 32 bits.
 */
typedef uint32_t time_usecs_t;

/**
 * @brief   Type of time conversion variable.
 * @note    This type must have double width than other time types, it is
 *          only used internally for conversions.
 */
typedef uint64_t time_conv_t;

#else
typedef uint16_t time_secs_t;
typedef uint16_t time_msecs_t;
typedef uint16_t time_usecs_t;
typedef uint32_t time_conv_t;
#endif

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @name    Fast time conversion utilities
 * @{
 */
/**
 * @brief   Seconds to time interval.
 * @details Converts from seconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 * @note    Use of this macro for large values is not secure because
 *          integer overflows, make sure your value can be correctly
 *          converted.
 *
 * @param[in] secs      number of seconds
 * @return              The number of ticks.
 *
 * @api
 */
#define TIME_S2I(secs)                                                      \
  ((sysinterval_t)((time_conv_t)(secs) * (time_conv_t)CH_CFG_ST_FREQUENCY))

/**
 * @brief   Milliseconds to time interval.
 * @details Converts from milliseconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 * @note    Use of this macro for large values is not secure because
 *          integer overflows, make sure your value can be correctly
 *          converted.
 *
 * @param[in] msecs     number of milliseconds
 * @return              The number of ticks.
 *
 * @api
 */
#define TIME_MS2I(msecs)                                                    \
  ((sysinterval_t)((((time_conv_t)(msecs) *                                 \
                     (time_conv_t)CH_CFG_ST_FREQUENCY) +                    \
                    (time_conv_t)999) / (time_conv_t)1000))

/**
 * @brief   Microseconds to time interval.
 * @details Converts from microseconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 * @note    Use of this macro for large values is not secure because
 *          integer overflows, make sure your value can be correctly
 *          converted.
 *
 * @param[in] usecs     number of microseconds
 * @return              The number of ticks.
 *
 * @api
 */
#define TIME_US2I(usecs)                                                    \
  ((sysinterval_t)((((time_conv_t)(usecs) *                                 \
                     (time_conv_t)CH_CFG_ST_FREQUENCY) +                    \
                    (time_conv_t)999999) / (time_conv_t)1000000))

/**
 * @brief   Time interval to seconds.
 * @details Converts from system ticks number to seconds.
 * @note    The result is rounded up to the next second boundary.
 * @note    Use of this macro for large values is not secure because
 *          integer overflows, make sure your value can be correctly
 *          converted.
 *
 * @param[in] interval  interval in ticks
 * @return              The number of seconds.
 *
 * @api
 */
#define TIME_I2S(interval)                                                  \
  (time_secs_t)(((time_conv_t)(interval) +                                  \
                 (time_conv_t)CH_CFG_ST_FREQUENCY -                         \
                 (time_conv_t)1) / (time_conv_t)CH_CFG_ST_FREQUENCY)

/**
 * @brief   Time interval to milliseconds.
 * @details Converts from system ticks number to milliseconds.
 * @note    The result is rounded up to the next millisecond boundary.
 * @note    Use of this macro for large values is not secure because
 *          integer overflows, make sure your value can be correctly
 *          converted.
 *
 * @param[in] interval  interval in ticks
 * @return              The number of milliseconds.
 *
 * @api
 */
#define TIME_I2MS(interval)                                                 \
  (time_msecs_t)((((time_conv_t)(interval) * (time_conv_t)1000) +           \
                  (time_conv_t)CH_CFG_ST_FREQUENCY - (time_conv_t)1) /      \
                 (time_conv_t)CH_CFG_ST_FREQUENCY)

/**
 * @brief   Time interval to microseconds.
 * @details Converts from system ticks number to microseconds.
 * @note    The result is rounded up to the next microsecond boundary.
 * @note    Use of this macro for large values is not secure because
 *          integer overflows, make sure your value can be correctly
 *          converted.
 *
 * @param[in] interval  interval in ticks
 * @return              The number of microseconds.
 *
 * @api
 */
#define TIME_I2US(interval)                                                 \
  (time_usecs_t)((((time_conv_t)(interval) * (time_conv_t)1000000) +        \
                  (time_conv_t)CH_CFG_ST_FREQUENCY - (time_conv_t)1) /      \
                 (time_conv_t)CH_CFG_ST_FREQUENCY)
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name    Secure time conversion utilities
 * @{
 */
/**
 * @brief   Seconds to time interval.
 * @details Converts from seconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] secs      number of seconds
 * @return              The number of ticks.
 *
 * @special
 */
static inline sysinterval_t chTimeS2I(time_secs_t secs) {
  time_conv_t ticks;

  ticks = (time_conv_t)secs * (time_conv_t)CH_CFG_ST_FREQUENCY;

  chDbgAssert(ticks <= (time_conv_t)TIME_MAX_INTERVAL,
              "conversion overflow");

  return (sysinterval_t)ticks;
}

/**
 * @brief   Milliseconds to time interval.
 * @details Converts from milliseconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] msec      number of milliseconds
 * @return              The number of ticks.
 *
 * @special
 */
static inline sysinterval_t chTimeMS2I(time_msecs_t msec) {
  time_conv_t ticks;

  ticks = (((time_conv_t)msec * (time_conv_t)CH_CFG_ST_FREQUENCY) +
           (time_conv_t)999) / (time_conv_t)1000;

  chDbgAssert(ticks <= (time_conv_t)TIME_MAX_INTERVAL,
              "conversion overflow");

  return (sysinterval_t)ticks;
}

/**
 * @brief   Microseconds to time interval.
 * @details Converts from microseconds to system ticks number.
 * @note    The result is rounded upward to the next tick boundary.
 *
 * @param[in] usec      number of microseconds
 * @return              The number of ticks.
 *
 * @special
 */
static inline sysinterval_t chTimeUS2I(time_usecs_t usec) {
  time_conv_t ticks;

  ticks = (((time_conv_t)usec * (time_conv_t)CH_CFG_ST_FREQUENCY) +
           (time_conv_t)999999) / (time_conv_t)1000000;

  chDbgAssert(ticks <= (time_conv_t)TIME_MAX_INTERVAL,
              "conversion overflow");

  return (sysinterval_t)ticks;
}

/**
 * @brief   Time interval to seconds.
 * @details Converts from system interval to seconds.
 * @note    The result is rounded up to the next second boundary.
 *
 * @param[in] interval  interval in ticks
 * @return              The number of seconds.
 *
 * @special
 */
static inline time_secs_t chTimeI2S(sysinterval_t interval) {
  time_conv_t secs;

  secs = ((time_conv_t)interval +
          (time_conv_t)CH_CFG_ST_FREQUENCY -
          (time_conv_t)1) / (time_conv_t)CH_CFG_ST_FREQUENCY;

  chDbgAssert(secs < (time_conv_t)((time_secs_t)-1),
              "conversion overflow");

  return (time_secs_t)secs;
}

/**
 * @brief   Time interval to milliseconds.
 * @details Converts from system interval to milliseconds.
 * @note    The result is rounded up to the next millisecond boundary.
 *
 * @param[in] interval  interval in ticks
 * @return              The number of milliseconds.
 *
 * @special
 */
static inline time_msecs_t chTimeI2MS(sysinterval_t interval) {
  time_conv_t msecs;

  msecs = (((time_conv_t)interval * (time_conv_t)1000) +
           (time_conv_t)CH_CFG_ST_FREQUENCY - (time_conv_t)1) /
          (time_conv_t)CH_CFG_ST_FREQUENCY;

  chDbgAssert(msecs < (time_conv_t)((time_msecs_t)-1),
              "conversion overflow");

  return (time_msecs_t)msecs;
}

/**
 * @brief   Time interval to microseconds.
 * @details Converts from system interval to microseconds.
 * @note    The result is rounded up to the next microsecond boundary.
 *
 * @param[in] interval  interval in ticks
 * @return              The number of microseconds.
 *
 * @special
 */
static inline time_usecs_t chTimeI2US(sysinterval_t interval) {
  time_conv_t usecs;

  usecs = (((time_conv_t)interval * (time_conv_t)1000000) +
           (time_conv_t)CH_CFG_ST_FREQUENCY - (time_conv_t)1) /
          (time_conv_t)CH_CFG_ST_FREQUENCY;

  chDbgAssert(usecs <= (time_conv_t)((time_usecs_t)-1),
              "conversion overflow");

  return (time_usecs_t)usecs;
}

/**
 * @brief   Adds an interval to a system time returning a system time.
 *
 * @param[in] systime   base system time
 * @param[in] interval  interval to be added
 * @return              The new system time.
 *
 * @xclass
 */
static inline systime_t chTimeAddX(systime_t systime,
                                   sysinterval_t interval) {

#if CH_CFG_ST_RESOLUTION != CH_CFG_INTERVALS_SIZE
  chDbgCheck(interval <= (sysinterval_t)TIME_MAX_SYSTIME);
#endif

  return systime + (systime_t)interval;
}

/**
 * @brief   Subtracts two system times returning an interval.
 *
 * @param[in] start     first system time
 * @param[in] end       second system time
 * @return              The interval representing the time difference.
 *
 * @xclass
 */
static inline sysinterval_t chTimeDiffX(systime_t start, systime_t end) {

  /*lint -save -e9033 [10.8] This cast is required by the operation, it is
    known that the destination type can be wider.*/
  return (sysinterval_t)((systime_t)(end - start));
  /*lint -restore*/
}

/**
 * @brief   Checks if the specified time is within the specified time range.
 * @note    When start==end then the function returns always false because the
 *          time window has zero size.
 *
 * @param[in] time      the time to be verified
 * @param[in] start     the start of the time window (inclusive)
 * @param[in] end       the end of the time window (non inclusive)
 * @retval true         if the current time is within the specified
 *                      time window.
 * @retval false        if the current time is not within the specified
 *                      time window.
 *
 * @xclass
 */
static inline bool chTimeIsInRangeX(systime_t time,
                                    systime_t start,
                                    systime_t end) {

  return (bool)((systime_t)((systime_t)time - (systime_t)start) <
                (systime_t)((systime_t)end - (systime_t)start));
}

/**
 * @brief   Adds an interval to a time stamp returning a time stamp.
 *
 * @param[in] stamp     base time stamp
 * @param[in] interval  interval to be added
 * @return              The new time stamp.
 *
 * @xclass
 */
static inline systimestamp_t chTimeStampAddX(systimestamp_t stamp,
                                             sysinterval_t interval) {

  return stamp + (systimestamp_t)interval;
}

/**
 * @brief   Subtracts two time stamps returning an interval.
 * @note    Intervals can then be used for converting in absolute time.
 *
 * @param[in] start     first time stamp
 * @param[in] end       second time stamp
 * @return              The interval representing the time stamps difference.
 *
 * @xclass
 */
static inline sysinterval_t chTimeStampDiffX(systimestamp_t start,
                                             systimestamp_t end) {
  systimestamp_t diff;

  /* Time difference as a wide time stamp.*/
  diff = end - start;

  /*lint -save -e685 [14.3] This condition becomes always true when both
    types have the same width, it is fine, this is an assertion.*/
  chDbgAssert(diff <= (systimestamp_t)((sysinterval_t)-1),
              "conversion overflow");
  /*lint -restore*/

  /*lint -save -e9033 [10.8] This cast is required by the operation, it is
    known that the destination type can be wider.*/
  return (sysinterval_t)diff;
  /*lint -restore*/
}

/**
 * @brief   Checks if the specified time stamp is within the specified time
 *          stamps range.
 * @note    When start==end then the function returns always false because the
 *          time window has zero size.
 *
 * @param[in] stamp     the time stamp to be verified
 * @param[in] start     the start of the time stamp window (inclusive)
 * @param[in] end       the end of the time stamp window (non inclusive)
 * @retval true         if the current time stamp is within the specified
 *                      time stamp window.
 * @retval false        if the current time stamp is not within the specified
 *                      time stamp window.
 *
 * @xclass
 */
static inline bool chTimeStampIsInRangeX(systimestamp_t stamp,
                                         systimestamp_t start,
                                         systimestamp_t end) {

  return (bool)((systimestamp_t)((systimestamp_t)stamp - (systimestamp_t)start) <
                (systimestamp_t)((systimestamp_t)end - (systimestamp_t)start));
}
/** @} */

#endif /* CHTIME_H */

/** @} */
```
