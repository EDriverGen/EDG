# Raw RTOS/Bus Pack: chibios / uart_polling

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

## Source: `data/rtos/chibios/os/hal/include/hal_serial.h`

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
 * @file    hal_serial.h
 * @brief   Serial Driver macros and structures.
 *
 * @addtogroup SERIAL
 * @{
 */

#ifndef HAL_SERIAL_H
#define HAL_SERIAL_H

#if (HAL_USE_SERIAL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Serial status flags (legacy)
 * @{
 */
#define SD_PARITY_ERROR         CHN_PARITY_ERROR
#define SD_FRAMING_ERROR        CHN_FRAMING_ERROR
#define SD_OVERRUN_ERROR        CHN_OVERRUN_ERROR
#define SD_NOISE_ERROR          CHN_NOISE_ERROR
#define SD_BREAK_DETECTED       CHN_BREAK_DETECTED
#define SD_QUEUE_FULL_ERROR     CHN_BUFFER_FULL_ERROR
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Serial configuration options
 * @{
 */
/**
 * @brief   Default bit rate.
 * @details Configuration parameter, this is the baud rate selected for the
 *          default configuration.
 */
#if !defined(SERIAL_DEFAULT_BITRATE) || defined(__DOXYGEN__)
#define SERIAL_DEFAULT_BITRATE      38400
#endif

/**
 * @brief   Serial buffers size.
 * @details Configuration parameter, you can change the depth of the queue
 *          buffers depending on the requirements of your application.
 * @note    The default is 16 bytes for both the transmission and receive
 *          buffers.
 * @note    This is a global setting and it can be overridden by low level
 *          driver specific settings.
 */
#if !defined(SERIAL_BUFFERS_SIZE) || defined(__DOXYGEN__)
#define SERIAL_BUFFERS_SIZE         16
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Driver state machine possible states.
 */
typedef enum {
  SD_UNINIT = 0,                    /**< Not initialized.                   */
  SD_STOP = 1,                      /**< Stopped.                           */
  SD_READY = 2                      /**< Ready.                             */
} sdstate_t;

/**
 * @brief   Structure representing a serial driver.
 */
typedef struct hal_serial_driver SerialDriver;

#include "hal_serial_lld.h"

/**
 * @brief   @p SerialDriver specific methods.
 */
#define _serial_driver_methods                                              \
  _base_asynchronous_channel_methods

/**
 * @extends BaseAsynchronousChannelVMT
 *
 * @brief   @p SerialDriver virtual methods table.
 */
struct SerialDriverVMT {
  _serial_driver_methods
};

/**
 * @extends BaseAsynchronousChannel
 *
 * @brief   Full duplex serial driver class.
 * @details This class extends @p BaseAsynchronousChannel by adding physical
 *          I/O queues.
 */
struct hal_serial_driver {
  /** @brief Virtual Methods Table.*/
  const struct SerialDriverVMT *vmt;
  _serial_driver_data
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Direct write to a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly on the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         the byte value to be written in the output queue
 * @return              The operation status.
 * @retval MSG_OK       if the operation succeeded.
 * @retval MSG_TIMEOUT  if the queue is full.
 *
 * @iclass
 */
#define sdPutI(sdp, b) oqPutI(&(sdp)->oqueue, b)

/**
 * @brief   Direct write to a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly on the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         the byte value to be written in the output queue
 * @return              The operation status.
 * @retval MSG_OK       if the operation succeeded.
 * @retval MSG_RESET    if the @p SerialDriver has been stopped.
 *
 * @api
 */
#define sdPut(sdp, b) oqPut(&(sdp)->oqueue, b)

/**
 * @brief   Direct write to a @p SerialDriver with timeout specification.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly on the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         the byte value to be written in the output queue
 * @param[in] t         the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 * @return              The operation status.
 * @retval MSG_OK       if the operation succeeded.
 * @retval MSG_TIMEOUT  if the specified time expired.
 * @retval MSG_RESET    if the @p SerialDriver has been stopped.
 *
 * @api
 */
#define sdPutTimeout(sdp, b, t) oqPutTimeout(&(sdp)->oqueue, b, t)

/**
 * @brief   Direct read from a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @return              A byte value from the input queue.
 * @retval MSG_TIMEOUT  if the queue is empty.
 *
 * @iclass
 */
#define sdGetI(sdp) iqGetI(&(sdp)->iqueue)

/**
 * @brief   Direct read from a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @return              A byte value from the input queue.
 * @retval MSG_RESET    if the @p SerialDriver has been stopped.
 *
 * @api
 */
#define sdGet(sdp) iqGet(&(sdp)->iqueue)

/**
 * @brief   Direct read from a @p SerialDriver with timeout specification.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] t         the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 * @return              A byte value from the input queue.
 * @retval MSG_TIMEOUT  if the specified time expired.
 * @retval MSG_RESET    if the @p SerialDriver has been stopped.
 *
 * @api
 */
#define sdGetTimeout(sdp, t) iqGetTimeout(&(sdp)->iqueue, t)

/**
 * @brief   Direct non-blocking write to a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly to the output queue. This is faster but cannot
 *          be used to write from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @return              The number of bytes effectively transferred.
 *
 * @iclass
 */
#define sdWriteI(sdp, b, n) oqWriteI(&(sdp)->oqueue, b, n)

/**
 * @brief   Direct blocking write to a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly to the output queue. This is faster but cannot
 *          be used to write from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 *
 * @api
 */
#define sdWrite(sdp, b, n) oqWriteTimeout(&(sdp)->oqueue, b, n, TIME_INFINITE)

/**
 * @brief   Direct blocking write to a @p SerialDriver with timeout
 *          specification.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly to the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @param[in] t         the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 * @return              The number of bytes effectively transferred.
 *
 * @api
 */
#define sdWriteTimeout(sdp, b, n, t)                                        \
  oqWriteTimeout(&(sdp)->oqueue, b, n, t)

/**
 * @brief   Direct non-blocking write to a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly to the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 *
 * @api
 */
#define sdAsynchronousWrite(sdp, b, n)                                      \
  oqWriteTimeout(&(sdp)->oqueue, b, n, TIME_IMMEDIATE)

/**
 * @brief   Direct non-blocking read from a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @return              The number of bytes effectively transferred.
 *
 * @iclass
 */
#define sdReadI(sdp, b, n) iqReadI(&(sdp)->iqueue, b, n)

/**
 * @brief   Direct blocking read from a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 *
 * @api
 */
#define sdRead(sdp, b, n) iqReadTimeout(&(sdp)->iqueue, b, n, TIME_INFINITE)

/**
 * @brief   Direct blocking read from a @p SerialDriver with timeout
 *          specification.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @param[in] t         the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 * @return              The number of bytes effectively transferred.
 *
 * @api
 */
#define sdReadTimeout(sdp, b, n, t) iqReadTimeout(&(sdp)->iqueue, b, n, t)

/**
 * @brief   Direct non-blocking read from a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] b         pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @return              The number of bytes effectively transferred.
 *
 * @api
 */
#define sdAsynchronousRead(sdp, b, n)                                       \
  iqReadTimeout(&(sdp)->iqueue, b, n, TIME_IMMEDIATE)
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void sdInit(void);
#if !defined(SERIAL_ADVANCED_BUFFERING_SUPPORT) ||                          \
    (SERIAL_ADVANCED_BUFFERING_SUPPORT == FALSE)
  void sdObjectInit(SerialDriver *sdp, qnotify_t inotify, qnotify_t onotify);
#else
  void sdObjectInit(SerialDriver *sdp);
#endif
  msg_t sdStart(SerialDriver *sdp, const SerialConfig *config);
  void sdStop(SerialDriver *sdp);
  void sdIncomingDataI(SerialDriver *sdp, uint8_t b);
  msg_t sdRequestDataI(SerialDriver *sdp);
  bool sdPutWouldBlock(SerialDriver *sdp);
  bool sdGetWouldBlock(SerialDriver *sdp);
  msg_t sdControl(SerialDriver *sdp, unsigned int operation, void *arg);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL == TRUE */

#endif /* HAL_SERIAL_H */

/** @} */
```

## Source: `data/rtos/chibios/os/hal/include/hal_uart.h`

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
 * @file    hal_uart.h
 * @brief   UART Driver macros and structures.
 *
 * @addtogroup UART
 * @{
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#if (HAL_USE_UART == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    UART status flags
 * @{
 */
#define UART_NO_ERROR           0   /**< @brief No pending conditions.      */
#define UART_PARITY_ERROR       4   /**< @brief Parity error happened.      */
#define UART_FRAMING_ERROR      8   /**< @brief Framing error happened.     */
#define UART_OVERRUN_ERROR      16  /**< @brief Overflow happened.          */
#define UART_NOISE_ERROR        32  /**< @brief Noise on the line.          */
#define UART_BREAK_DETECTED     64  /**< @brief Break detected.             */
/** @} */

/**
 * @name    UART error conditions
 * @{
 */
#define UART_ERR_NOT_ACTIVE     (size_t)-1
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    UART configuration options
 * @{
 */
/**
 * @brief   Enables synchronous APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(UART_USE_WAIT) || defined(__DOXYGEN__)
#define UART_USE_WAIT                       FALSE
#endif

/**
 * @brief   Enables the @p uartAcquireBus() and @p uartReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(UART_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define UART_USE_MUTUAL_EXCLUSION           FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  UART_UNINIT = 0,                  /**< Not initialized.                   */
  UART_STOP = 1,                    /**< Stopped.                           */
  UART_READY = 2                    /**< Ready.                             */
} uartstate_t;

/**
 * @brief   Transmitter state machine states.
 */
typedef enum {
  UART_TX_IDLE = 0,                 /**< Not transmitting.                  */
  UART_TX_ACTIVE = 1                /**< Transmitting.                      */
} uarttxstate_t;

/**
 * @brief   Receiver state machine states.
 */
typedef enum {
  UART_RX_IDLE = 0,                 /**< Not receiving.                     */
  UART_RX_ACTIVE = 1                /**< Receiving.                         */
} uartrxstate_t;

#include "hal_uart_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Low level driver helper macros
 * @{
 */
#if (UART_USE_WAIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Wakes up the waiting thread in case of early TX complete.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_wakeup_tx1_isr(uartp) {                                       \
  if ((uartp)->early == true) {                                             \
    osalSysLockFromISR();                                                   \
    osalThreadResumeI(&(uartp)->threadtx, MSG_OK);                          \
    osalSysUnlockFromISR();                                                 \
  }                                                                         \
}
#else /* !UART_USE_WAIT */
#define _uart_wakeup_tx1_isr(uartp)
#endif /* !UART_USE_WAIT */

#if (UART_USE_WAIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Wakes up the waiting thread in case of late TX complete.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_wakeup_tx2_isr(uartp) {                                       \
  if ((uartp)->early == false) {                                            \
    osalSysLockFromISR();                                                   \
    osalThreadResumeI(&(uartp)->threadtx, MSG_OK);                          \
    osalSysUnlockFromISR();                                                 \
  }                                                                         \
}
#else /* !UART_USE_WAIT */
#define _uart_wakeup_tx2_isr(uartp)
#endif /* !UART_USE_WAIT */

#if (UART_USE_WAIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Wakes up the waiting thread in case of RX complete.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_wakeup_rx_complete_isr(uartp) {                               \
  osalSysLockFromISR();                                                     \
  osalThreadResumeI(&(uartp)->threadrx, MSG_OK);                            \
  osalSysUnlockFromISR();                                                   \
}
#else /* !UART_USE_WAIT */
#define _uart_wakeup_rx_complete_isr(uartp)
#endif /* !UART_USE_WAIT */

#if (UART_USE_WAIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Wakes up the waiting thread in case of RX error.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_wakeup_rx_error_isr(uartp) {                                  \
  osalSysLockFromISR();                                                     \
  osalThreadResumeI(&(uartp)->threadrx, MSG_RESET);                         \
  osalSysUnlockFromISR();                                                   \
}
#else /* !UART_USE_WAIT */
#define _uart_wakeup_rx_error_isr(uartp)
#endif /* !UART_USE_WAIT */

#if (UART_USE_WAIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Wakes up the waiting thread in case of RX character match.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_wakeup_rx_cm_isr(uartp) {                                     \
  osalSysLockFromISR();                                                     \
  osalThreadResumeI(&(uartp)->threadrx, MSG_TIMEOUT);                       \
  osalSysUnlockFromISR();                                                   \
}
#else /* !UART_USE_WAIT */
#define _uart_wakeup_rx_cm_isr(uartp)
#endif /* !UART_USE_WAIT */

#if (UART_USE_WAIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Wakes up the waiting thread in case of RX timeout.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_wakeup_rx_timeout_isr(uartp) {                                \
  osalSysLockFromISR();                                                     \
  osalThreadResumeI(&(uartp)->threadrx, MSG_TIMEOUT);                       \
  osalSysUnlockFromISR();                                                   \
}
#else /* !UART_USE_WAIT */
#define _uart_wakeup_rx_timeout_isr(uartp)
#endif /* !UART_USE_WAIT */

/**
 * @brief   Common ISR code for early TX.
 * @details This code handles the portable part of the ISR code:
 *          - Callback invocation.
 *          - Waiting thread wakeup, if any.
 *          - Driver state transitions.
 *          .
 * @note    This macro is meant to be used in the low level drivers
 *          implementation only.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_tx1_isr_code(uartp) {                                         \
  (uartp)->txstate = UART_TX_IDLE;                                          \
  if ((uartp)->config->txend1_cb != NULL) {                                 \
    (uartp)->config->txend1_cb(uartp);                                      \
  }                                                                         \
  _uart_wakeup_tx1_isr(uartp);                                              \
}

/**
 * @brief   Common ISR code for late TX.
 * @details This code handles the portable part of the ISR code:
 *          - Callback invocation.
 *          - Waiting thread wakeup, if any.
 *          - Driver state transitions.
 *          .
 * @note    This macro is meant to be used in the low level drivers
 *          implementation only.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_tx2_isr_code(uartp) {                                         \
  if ((uartp)->config->txend2_cb != NULL) {                                 \
    (uartp)->config->txend2_cb(uartp);                                      \
  }                                                                         \
  _uart_wakeup_tx2_isr(uartp);                                              \
}

/**
 * @brief   Common ISR code for RX complete.
 * @details This code handles the portable part of the ISR code:
 *          - Callback invocation.
 *          - Waiting thread wakeup, if any.
 *          - Driver state transitions.
 *          .
 * @note    This macro is meant to be used in the low level drivers
 *          implementation only.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_rx_complete_isr_code(uartp) {                                 \
  (uartp)->rxstate = UART_RX_IDLE;                                          \
  uart_enter_rx_idle_loop(uartp);                                           \
  if ((uartp)->config->rxend_cb != NULL) {                                  \
    (uartp)->config->rxend_cb(uartp);                                       \
  }                                                                         \
  _uart_wakeup_rx_complete_isr(uartp);                                      \
}

/**
 * @brief   Common ISR code for RX error.
 * @details This code handles the portable part of the ISR code:
 *          - Callback invocation.
 *          - Waiting thread wakeup, if any.
 *          - Driver state transitions.
 *          .
 * @note    This macro is meant to be used in the low level drivers
 *          implementation only.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] errors    mask of errors to be reported
 *
 * @notapi
 */
#define _uart_rx_error_isr_code(uartp, errors) {                            \
  if ((uartp)->config->rxerr_cb != NULL) {                                  \
    (uartp)->config->rxerr_cb(uartp, errors);                               \
  }                                                                         \
  _uart_wakeup_rx_error_isr(uartp);                                         \
}

/**
 * @brief   Common ISR code for RX on idle.
 * @details This code handles the portable part of the ISR code:
 *          - Callback invocation.
 *          - Waiting thread wakeup, if any.
 *          - Driver state transitions.
 *          .
 * @note    This macro is meant to be used in the low level drivers
 *          implementation only.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_rx_idle_code(uartp) {                                         \
  if ((uartp)->config->rxchar_cb != NULL)                                   \
    (uartp)->config->rxchar_cb(uartp, (uartp)->rxbuf);                      \
}

/**
 * @brief   Timeout ISR code for receiver.
 * @details This code handles the portable part of the ISR code:
 *          - Callback invocation.
 *          - Waiting thread wakeup, if any.
 *          - Driver state transitions.
 *          .
 * @note    This macro is meant to be used in the low level drivers
 *          implementation only.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_timeout_isr_code(uartp) {                                     \
  if ((uartp)->config->timeout_cb != NULL) {                                \
    (uartp)->config->timeout_cb(uartp);                                     \
  }                                                                         \
  _uart_wakeup_rx_timeout_isr(uartp);                                       \
}

/**
 * @brief   Character match ISR code for receiver.
 * @details This code handles the portable part of the ISR code:
 *          - Callback invocation.
 *          - Waiting thread wakeup, if any.
 *          - Driver state transitions.
 *          .
 * @note    This macro is meant to be used in the low level drivers
 *          implementation only.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
#define _uart_rx_char_match_isr_code(uartp) {                               \
  if ((uartp)->config->rx_cm_cb != NULL) {                                  \
    (uartp)->config->rx_cm_cb(uartp);                                       \
  }                                                                         \
  _uart_wakeup_rx_cm_isr(uartp);                                            \
}

/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void uartInit(void);
  void uartObjectInit(UARTDriver *uartp);
  msg_t uartStart(UARTDriver *uartp, const UARTConfig *config);
  void uartStop(UARTDriver *uartp);
  void uartStartSend(UARTDriver *uartp, size_t n, const void *txbuf);
  void uartStartSendI(UARTDriver *uartp, size_t n, const void *txbuf);
  size_t uartStopSend(UARTDriver *uartp);
  size_t uartStopSendI(UARTDriver *uartp);
  void uartStartReceive(UARTDriver *uartp, size_t n, void *rxbuf);
  void uartStartReceiveI(UARTDriver *uartp, size_t n, void *rxbuf);
  size_t uartStopReceive(UARTDriver *uartp);
  size_t uartStopReceiveI(UARTDriver *uartp);
#if UART_USE_WAIT == TRUE
  msg_t uartSendTimeout(UARTDriver *uartp, size_t *np,
                        const void *txbuf, sysinterval_t timeout);
  msg_t uartSendFullTimeout(UARTDriver *uartp, size_t *np,
                            const void *txbuf, sysinterval_t timeout);
  msg_t uartReceiveTimeout(UARTDriver *uartp, size_t *np,
                           void *rxbuf, sysinterval_t timeout);
#endif
#if UART_USE_MUTUAL_EXCLUSION == TRUE
  void uartAcquireBus(UARTDriver *uartp);
  void uartReleaseBus(UARTDriver *uartp);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_UART == TRUE */

#endif /* HAL_UART_H */

/** @} */
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
