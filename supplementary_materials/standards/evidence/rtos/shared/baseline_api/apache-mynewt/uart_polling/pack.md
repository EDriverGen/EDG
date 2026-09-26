# Raw RTOS/Bus Pack: apache-mynewt / uart_polling

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/apache-mynewt/kernel/os/include/os/mynewt.h`

```c
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_OS_MYNEWT_
#define H_OS_MYNEWT_

#include "syscfg/syscfg.h"
#include "sysdown/sysdown.h"
#include "sysinit/sysinit.h"
#include "sysflash/sysflash.h"
#include "os/os.h"
#include "defs/error.h"
#include "sys/debug_panic.h"

/* Only include the logcfg header if this version of newt can generate it. */
#if MYNEWT_VAL(NEWT_FEATURE_LOGCFG)
#include "logcfg/logcfg.h"
#endif

#endif
```

## Source: `data/rtos/apache-mynewt/hw/hal/include/hal/hal_uart.h`

```c
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


/**
 * @addtogroup HAL
 * @{
 *   @defgroup HALUart HAL UART
 *   @{
 */

#ifndef H_HAL_UART_H_
#define H_HAL_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>


/**
 * Function prototype for UART driver to ask for more data to send.
 * Returns -1 if no more data is available for TX.
 * Driver must call this with interrupts disabled.
 */
typedef int (*hal_uart_tx_char)(void *arg);

/**
 * Function prototype for UART driver to report that transmission is
 * complete. This should be called when transmission of last byte is
 * finished.
 * Driver must call this with interrupts disabled.
 */
typedef void (*hal_uart_tx_done)(void *arg);

/**
 * Function prototype for UART driver to report incoming byte of data.
 * Returns -1 if data was dropped.
 * Driver must call this with interrupts disabled.
 */
typedef int (*hal_uart_rx_char)(void *arg, uint8_t byte);

/**
 * Initializes given uart. Mapping of logical UART number to physical
 * UART/GPIO pins is in BSP.
 */
int hal_uart_init_cbs(int uart, hal_uart_tx_char tx_func,
  hal_uart_tx_done tx_done, hal_uart_rx_char rx_func, void *arg);

enum hal_uart_parity {
    /** No Parity */
    HAL_UART_PARITY_NONE = 0,
    /** Odd parity */
    HAL_UART_PARITY_ODD = 1,
    /** Even parity */
    HAL_UART_PARITY_EVEN = 2
};

enum hal_uart_flow_ctl {
    /** No Flow Control */
    HAL_UART_FLOW_CTL_NONE = 0,
    /** RTS/CTS */
    HAL_UART_FLOW_CTL_RTS_CTS = 1
};

/**
 * Initialize the HAL uart.
 *
 * @param uart  The uart number to configure
 * @param cfg   Hardware specific uart configuration.  This is passed from BSP
 *              directly to the MCU specific driver.
 *
 * @return 0 on success, non-zero error code on failure
 */
int hal_uart_init(int uart, void *cfg);

/**
 * Applies given configuration to UART.
 *
 * @param uart The UART number to configure
 * @param speed The baudrate in bps to configure
 * @param databits The number of databits to send per byte
 * @param stopbits The number of stop bits to send
 * @param parity The UART parity
 * @param flow_ctl Flow control settings on the UART
 *
 * @return 0 on success, non-zero error code on failure
 */
int hal_uart_config(int uart, int32_t speed, uint8_t databits, uint8_t stopbits,
  enum hal_uart_parity parity, enum hal_uart_flow_ctl flow_ctl);

/**
 * Close UART port. Can call hal_uart_config() with different settings after
 * calling this.
 *
 * @param uart The UART number to close
 */
int hal_uart_close(int uart);

/**
 * More data queued for transmission. UART driver will start asking for that
 * data.
 *
 * @param uart The UART number to start TX on
 */
void hal_uart_start_tx(int uart);

/**
 * Upper layers have consumed some data, and are now ready to receive more.
 * This is meaningful after uart_rx_char callback has returned -1 telling
 * that no more data can be accepted.
 *
 * @param uart The UART number to begin RX on
 */
void hal_uart_start_rx(int uart);

/**
 * This is type of write where UART has to block until character has been sent.
 * Used when printing diag output from system crash.
 * Must be called with interrupts disabled.
 *
 * @param uart The UART number to TX on
 * @param byte The byte to TX on the UART
 */
void hal_uart_blocking_tx(int uart, uint8_t byte);

#ifdef __cplusplus
}
#endif


#endif /* H_HAL_UART_H_ */


/**
 *   @} HALUart
 * @} HAL
 */
```

## Source: `data/rtos/apache-mynewt/kernel/os/include/os/os_time.h`

```c
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*-
 * Copyright (c) 1982, 1986, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)time.h      8.5 (Berkeley) 5/4/95
 * $FreeBSD$
 */


/**
 * @addtogroup OSKernel
 * @{
 *   @defgroup OSTime Time
 *   @{
 */

#ifndef _OS_TIME_H
#define _OS_TIME_H

#include <stdbool.h>
#include <stdint.h>
#include "os/os_arch.h"
#include "os/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UINT32_MAX
/** Maximum value of 32-bit unsigned integer. */
#define UINT32_MAX  0xFFFFFFFFU
#endif

#ifndef INT32_MAX
/** Maximum value of 32-bit signed integer. */
#define INT32_MAX   0x7FFFFFFF
#endif

#ifdef OS_TICKS_PER_SEC
#warning "OS_TICKS_PER_SEC should be configured in syscfg"
#else
#if MYNEWT_VAL(OS_TICKS_PER_SEC)
#define OS_TICKS_PER_SEC        MYNEWT_VAL(OS_TICKS_PER_SEC)
#else
#error "Application, BSP or target must specify OS_TICKS_PER_SEC syscfg value"
#endif
#endif

/** Signed 32-bit system time type definition. */
typedef int32_t os_stime_t;

/** Unsigned 32-bit system time type definition. */
typedef uint32_t os_time_t;

/** Maximum value for os_time_t. */
#define OS_TIME_MAX UINT32_MAX

/** Maximum value for os_stime_t. */
#define OS_STIME_MAX INT32_MAX

/** Used to wait forever for events and mutexs */
#define OS_TIMEOUT_NEVER    (OS_TIME_MAX)


/**
 * Get the current OS time in ticks
 *
 * @return                      OS time in ticks
 */
os_time_t os_time_get(void);

/**
 * Move OS time forward ticks.
 *
 * @param ticks                 The number of ticks to move time forward.
 */
void os_time_advance(int ticks);

/**
 * Puts the current task to sleep for the specified number of os ticks. There
 * is no delay if ticks is 0.
 *
 * @param osticks               Number of ticks to delay (0 means no delay).
 */
void os_time_delay(os_time_t osticks);

/**
 * @defgroup OSTime_cmp_macros Helper macros for time comparisons
 * @{
 */

/**
 * Checks if time tick __t1 is less than time tick __t2.
 *
 * @param __t1                  The first time tick to compare.
 * @param __t2                  The second time tick to compare.
 *
 * @return                      True if __t1 is less than __t2;
 *                              false otherwise.
 */
#define OS_TIME_TICK_LT(__t1, __t2) ((os_stime_t) ((__t1) - (__t2)) < 0)

/**
 * Checks if time tick __t1 is greater than time tick __t2.
 *
 * @param __t1                  The first time tick to compare.
 * @param __t2                  The second time tick to compare.
 *
 * @return                      True if __t1 is greater than __t2;
 *                              false otherwise.
 */
#define OS_TIME_TICK_GT(__t1, __t2) ((os_stime_t) ((__t1) - (__t2)) > 0)

/**
 * Checks if time tick __t1 is greater than or equal to time tick __t2.
 *
 * @param __t1                  The first time tick to compare.
 * @param __t2                  The second time tick to compare.
 *
 * @return                      True if __t1 is greater than or equal to __t2;
 *                              false otherwise.
 */
#define OS_TIME_TICK_GEQ(__t1, __t2) ((os_stime_t) ((__t1) - (__t2)) >= 0)

/**
 * Checks if timeval __t1 is less than timeval __t2.
 *
 * @param __t1                  The first timeval to compare.
 * @param __t2                  The second timeval to compare.
 *
 * @return                      True if __t1 is less than __t2;
 *                              false otherwise.
 */
#define OS_TIMEVAL_LT(__t1, __t2) \
    (((__t1).tv_sec < (__t2).tv_sec) || \
     (((__t1).tv_sec == (__t2).tv_sec) && ((__t1).tv_usec < (__t2).tv_usec)))

/**
 * Checks if timeval __t1 is less than or equal to timeval __t2.
 *
 * @param __t1                  The first timeval to compare.
 * @param __t2                  The second timeval to compare.
 *
 * @return                      True if __t1 is less than or equal to __t2;
 *                              false otherwise.
 */
#define OS_TIMEVAL_LEQ(__t1, __t2) \
    (((__t1).tv_sec < (__t2).tv_sec) || \
     (((__t1).tv_sec == (__t2).tv_sec) && ((__t1).tv_usec <= (__t2).tv_usec)))

/**
 * Checks if timeval __t1 is greater than timeval __t2.
 *
 * @param __t1                  The first timeval to compare.
 * @param __t2                  The second timeval to compare.
 *
 * @return                      True if __t1 is greater than __t2;
 *                              false otherwise.
 */
#define OS_TIMEVAL_GT(__t1, __t2) \
    (((__t1).tv_sec > (__t2).tv_sec) || \
     (((__t1).tv_sec == (__t2).tv_sec) && ((__t1).tv_usec > (__t2).tv_usec)))

/**
 * Checks if timeval __t1 is greater than or equal to timeval __t2.
 *
 * @param __t1                  The first timeval to compare.
 * @param __t2                  The second timeval to compare.
 *
 * @return                      True if __t1 is greater than or equal to __t2;
 *                              false otherwise.
 */
#define OS_TIMEVAL_GEQ(__t1, __t2) \
    (((__t1).tv_sec > (__t2).tv_sec) || \
     (((__t1).tv_sec == (__t2).tv_sec) && ((__t1).tv_usec >= (__t2).tv_usec)))


/** @} */

/**
 * Structure representing time since Jan 1 1970 with microsecond
 * granularity
 */
struct os_timeval {
    /** Seconds */
    int64_t tv_sec;
    /** Microseconds within the second */
    int32_t tv_usec;
};

/** Structure representing a timezone offset */
struct os_timezone {
    /** Minutes west of GMT */
    int16_t tz_minuteswest;
    /** Daylight savings time correction (if any) */
    int16_t tz_dsttime;
};

/**
 * Represents a time change.  Passed to time change listeners when the current
 * time-of-day is set.
 */
struct os_time_change_info {
    /** UTC time prior to change. */
    const struct os_timeval *tci_prev_tv;
    /** Time zone prior to change. */
    const struct os_timezone *tci_prev_tz;
    /** UTC time after change. */
    const struct os_timeval *tci_cur_tv;
    /** Time zone after change. */
    const struct os_timezone *tci_cur_tz;
    /** True if the time was not set prior to change. */
    bool tci_newly_synced;
};

/**
 * Callback that is executed when the time-of-day is set.
 *
 * @param info                  Describes the time change that just occurred.
 * @param arg                   Optional argument correponding to listener.
 */
typedef void os_time_change_fn(const struct os_time_change_info *info,
                               void *arg);

/** Time change listener.  Notified when the time-of-day is set. */
struct os_time_change_listener {
    /** Callback invoked when the time-of-day is set. */
    os_time_change_fn *tcl_fn;
    /** Argument to be passed to the callback function. */
    void *tcl_arg;

    /** Next listener in the list. */
    STAILQ_ENTRY(os_time_change_listener) tcl_next;
};

/**
 * Add first two timeval arguments and place results in third timeval
 * argument.
 */
#define os_timeradd(tvp, uvp, vvp)                                      \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
                if ((vvp)->tv_usec >= 1000000) {                        \
                        (vvp)->tv_sec++;                                \
                        (vvp)->tv_usec -= 1000000;                      \
                }                                                       \
        } while (0)


/**
 * Subtract first two timeval arguments and place results in third timeval
 * argument.
 */
#define os_timersub(tvp, uvp, vvp)                                      \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)


/**
 * Set the time of day.  This does not modify os time, but rather just modifies
 * the offset by which we are tracking real time against os time.  This
 * function notifies all registered time change listeners.
 *
 * @param utctime               A timeval representing the UTC time we are
 *                                  setting
 * @param tz                    The time-zone to apply against the utctime being
 *                                  set.
 *
 * @return                      0 on success;
 *                              non-zero on failure.
 */
int os_settimeofday(struct os_timeval *utctime, struct os_timezone *tz);

/**
 * Get the current time of day.  Returns the time of day in UTC
 * into the utctime argument, and returns the timezone (if set) into
 * tz.
 *
 * @param utctime               The structure to put the UTC time of day into
 * @param tz                    The structure to put the timezone information
 *                                  into
 *
 * @return 0 on success, non-zero on failure
 */
int os_gettimeofday(struct os_timeval *utctime, struct os_timezone *tz);

/**
 * Indicates whether the time has been set.
 *
 * @return                      True if time is set;
 *                              false otherwise.
 */
bool os_time_is_set(void);

/**
 * Get time since boot in microseconds.
 *
 * @return                      Time since boot in microseconds
 */
int64_t os_get_uptime_usec(void);

/**
 * Get time since boot as os_timeval.
 *
 * @param tvp                   Structure to put the time since boot.
 */
void os_get_uptime(struct os_timeval *tvp);

/**
 * Converts milliseconds to OS ticks.
 *
 * @param ms                    The milliseconds input.
 * @param out_ticks             The OS ticks output.
 *
 * @return                      0 on success;
 *                              OS_EINVAL if the result is too large to fit in
 *                                  a uint32_t.
 */
int os_time_ms_to_ticks(uint32_t ms, os_time_t *out_ticks);

/**
 * Converts OS ticks to milliseconds.
 *
 * @param ticks                 The OS ticks input.
 * @param out_ms                The milliseconds output.
 *
 * @return                      0 on success;
 *                              OS_EINVAL if the result is too large to fit in
 *                                  a uint32_t.
 */
int os_time_ticks_to_ms(os_time_t ticks, uint32_t *out_ms);


/**
 * Converts milliseconds to OS ticks.
 *
 * This function does not check if conversion overflows and should be only used
 * in cases where input is known to be small enough not to overflow.
 *
 * @param ms                    The milliseconds input.
 *
 * @return                      The number of OS ticks.
 */
static inline os_time_t
os_time_ms_to_ticks32(uint32_t ms)
{
#if OS_TICKS_PER_SEC == 1000
    return ms;
#else
    return ((uint64_t)ms * OS_TICKS_PER_SEC) / 1000;
#endif
}

/**
 * Converts OS ticks to milliseconds.
 *
 * This function does not check if conversion overflows and should be only used
 * in cases where input is known to be small enough not to overflow.
 *
 * @param ticks                 The OS ticks input.
 *
 * @return                      The number of milliseconds.
 */
static inline uint32_t
os_time_ticks_to_ms32(os_time_t ticks)
{
#if OS_TICKS_PER_SEC == 1000
    return ticks;
#else
    return ((uint64_t)ticks * 1000) / OS_TICKS_PER_SEC;
#endif
}

/**
 * Registers a time change listener.  Whenever the time is set, all registered
 * listeners are notified.  The provided pointer is added to an internal list,
 * so the listener's lifetime must extend indefinitely (or until the listener
 * is removed).
 *
 * @note This function is not thread safe.  The following operations must be
 * kept exclusive:
 *     o Addition of listener
 *     o Removal of listener
 *     o Setting time
 *
 * @param listener              The listener to register.
 */
void os_time_change_listen(struct os_time_change_listener *listener);

/**
 * Unregisters a time change listener.
 *
 * @note This function is not thread safe.  The following operations must be
 * kept exclusive:
 *     o Addition of listener
 *     o Removal of listener
 *     o Setting time
 *
 * @param listener              The listener to unregister.
 *
 * @return                      0 on success;
 *                              non-zero error code on failure
 */
int os_time_change_remove(const struct os_time_change_listener *listener);

#ifdef __cplusplus
}
#endif

#endif /* _OS_TIME_H */


/**
 *   @} OSKernel
 * @} OSTime
 */
```

## Source: `data/rtos/apache-mynewt/kernel/os/include/os/os_cputime.h`

```c
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

 /**
  * @addtogroup OSKernel
  * @{
  *   @defgroup OSCPUTime High Resolution Timers
  *   @{
  */

#ifndef H_OS_CPUTIME_
#define H_OS_CPUTIME_

#include "syscfg/syscfg.h"
#include "os/queue.h"
#include "hal/hal_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NOTE: these definitions allow one to override the cputime frequency used.
 * The reason these definitions exist is to make the code more
 * efficient/smaller when CPUTIME counts at 1 MHz.
 *
 * For those who want a different cputime frequency, you can set the config
 * definition for OS_CPUTIME_FREQ to the desired frequency in your project,
 * target or bsp.
 */
#if MYNEWT_VAL(OS_CPUTIME_FREQ) == 15625      ||  \
    MYNEWT_VAL(OS_CPUTIME_FREQ) == 31250      ||  \
    MYNEWT_VAL(OS_CPUTIME_FREQ) == 62500      ||  \
    MYNEWT_VAL(OS_CPUTIME_FREQ) == 125000     ||  \
    MYNEWT_VAL(OS_CPUTIME_FREQ) == 250000     ||  \
    MYNEWT_VAL(OS_CPUTIME_FREQ) == 500000     ||  \
    MYNEWT_VAL(OS_CPUTIME_FREQ) == 1000000

#define OS_CPUTIME_FREQ_1MHZ

#elif MYNEWT_VAL(OS_CPUTIME_FREQ) == 256        ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 512        ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 1024       ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 2048       ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 4096       ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 8192       ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 16384      ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 32768      ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 65536      ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 131072     ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 262144     ||  \
      MYNEWT_VAL(OS_CPUTIME_FREQ) == 524288

#define OS_CPUTIME_FREQ_PWR2

#elif MYNEWT_VAL(OS_CPUTIME_FREQ) > 1000000

#define OS_CPUTIME_FREQ_HIGH

#else

#error "Invalid OS_CPUTIME_FREQ value.  Value must be one of a) a power of 2" \
       ">= 256Hz, or b) any value >= 1MHz"

#endif

#if defined(OS_CPUTIME_FREQ_HIGH)
/** CPUTIME data. */
struct os_cputime_data
{
    /** Number of ticks per usec */
    uint32_t ticks_per_usec;
};
/** Global instance of CPUTIME data. */
extern struct os_cputime_data g_os_cputime;
#endif

/**
 * @defgroup OSCPUTime_cmp_macros Helper macros to compare cputimes
 * @{
 */
/** evaluates to true if t1 is before t2 in time */
#define CPUTIME_LT(__t1, __t2) ((int32_t)   ((__t1) - (__t2)) < 0)
/** evaluates to true if t1 is after t2 in time */
#define CPUTIME_GT(__t1, __t2) ((int32_t)   ((__t1) - (__t2)) > 0)
/** evaluates to true if t1 is on or after t2 in time */
#define CPUTIME_GEQ(__t1, __t2) ((int32_t)  ((__t1) - (__t2)) >= 0)
/** evaluates to true if t1 is on or before t2 in time */
#define CPUTIME_LEQ(__t1, __t2) ((int32_t)  ((__t1) - (__t2)) <= 0)

/** @} */

/**
 * Initialize the cputime module. This must be called after os_init is called
 * and before any other timer API are used. This should be called only once
 * and should be called before the hardware timer is used.
 *
 * @param clock_freq            The desired cputime frequency, in hertz (Hz).
 *
 * @return                      0 on success;
 *                              -1 on error.
 */
int os_cputime_init(uint32_t clock_freq);

/**
 * Returns the low 32 bits of cputime.
 *
 * @return                      The lower 32 bits of cputime
 */
uint32_t os_cputime_get32(void);

#if !defined(OS_CPUTIME_FREQ_PWR2)
/**
 * Converts the given number of nanoseconds into cputime ticks.
 * Not defined if OS_CPUTIME_FREQ_PWR2 is defined.
 *
 * @param nsecs                 The number of nanoseconds to convert to ticks
 *
 * @return                      The number of ticks corresponding to 'nsecs'
 */
uint32_t os_cputime_nsecs_to_ticks(uint32_t nsecs);

/**
 * Convert the given number of ticks into nanoseconds.
 * Not defined if OS_CPUTIME_FREQ_PWR2 is defined.
 *
 * @param ticks                 The number of ticks to convert to nanoseconds.
 *
 * @return                      The number of nanoseconds corresponding to
 *                                  'ticks'
 */
uint32_t os_cputime_ticks_to_nsecs(uint32_t ticks);

/**
 * Wait until 'nsecs' nanoseconds has elapsed. This is a blocking delay.
 * Not defined if OS_CPUTIME_FREQ_PWR2 is defined.
 *
 *
 * @param nsecs                 The number of nanoseconds to wait.
 */
void os_cputime_delay_nsecs(uint32_t nsecs);
#endif

#if defined(OS_CPUTIME_FREQ_1MHZ)

static inline uint32_t
os_cputime_usecs_to_ticks(uint32_t usecs)
{
    return usecs / (1000000 / MYNEWT_VAL(OS_CPUTIME_FREQ));
}

static inline uint32_t
os_cputime_ticks_to_usecs(uint32_t ticks)
{
    return ticks * (1000000 / MYNEWT_VAL(OS_CPUTIME_FREQ));
}

#else

/**
 * Converts the given number of microseconds into cputime ticks.
 *
 * @param usecs                 The number of microseconds to convert to ticks
 *
 * @return                      The number of ticks corresponding to 'usecs'
 */
uint32_t os_cputime_usecs_to_ticks(uint32_t usecs);

/**
 * Convert the given number of ticks into microseconds.
 *
 * @param ticks                 The number of ticks to convert to microseconds.
 *
 * @return                      The number of microseconds corresponding to
 *                                  'ticks'
 */
uint32_t os_cputime_ticks_to_usecs(uint32_t ticks);
#endif

/**
 * Wait until the number of ticks has elapsed. This is a blocking delay.
 *
 * @param ticks                 The number of ticks to wait.
 */
void os_cputime_delay_ticks(uint32_t ticks);

/**
 * Wait until 'usecs' microseconds has elapsed. This is a blocking delay.
 *
 * @param usecs                 The number of usecs to wait.
 */
void os_cputime_delay_usecs(uint32_t usecs);

/**
 * Initialize a CPU timer, using the given HAL timer.
 *
 * @param timer                 The timer to initialize. Cannot be NULL.
 * @param fp                    The timer callback function. Cannot be NULL.
 * @param arg                   Pointer to data object to pass to timer.
 */
void os_cputime_timer_init(struct hal_timer *timer, hal_timer_cb fp,
        void *arg);

/**
 * Start a cputimer that will expire at 'cputime'. If cputime has already
 * passed, the timer callback will still be called (at interrupt context).
 *
 * @note                        This must be called when the timer is stopped.
 *
 * @param timer                 Pointer to timer to start. Cannot be NULL.
 * @param cputime               The cputime at which the timer should expire.
 *
 * @return                      0 on success;
 *                              EINVAL if timer already started or timer struct
 *                                  invalid
 *
 */
int os_cputime_timer_start(struct hal_timer *timer, uint32_t cputime);

/**
 * Sets a cpu timer that will expire 'usecs' microseconds from the current
 * cputime.
 *
 * @note                        This must be called when the timer is stopped.
 *
 * @param timer                 Pointer to timer. Cannot be NULL.
 * @param usecs                 The number of usecs from now at which the timer
 *                                  will expire.
 *
 * @return                      0 on success;
 *                              EINVAL if timer already started or timer struct
 *                                  invalid
 */
int os_cputime_timer_relative(struct hal_timer *timer, uint32_t usecs);

/**
 * Stops a cputimer from running. The timer is removed from the timer queue
 * and interrupts are disabled if no timers are left on the queue. Can be
 * called even if timer is not running.
 *
 * @param timer                 Pointer to cputimer to stop. Cannot be NULL.
 */
void os_cputime_timer_stop(struct hal_timer *timer);

#ifdef __cplusplus
}
#endif

#endif /* H_OS_CPUTIME_ */

/**
 *   @} OSCPUTime
 * @} OSKernel
 */
```

## Source: `data/rtos/apache-mynewt/kernel/os/include/os/os_error.h`

```c
/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_OS_ERROR_
#define H_OS_ERROR_

#include "syscfg/syscfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OS error enumerations */
enum os_error {
    OS_OK = 0,
    OS_ENOMEM = 1,
    OS_EINVAL = 2,
    OS_INVALID_PARM = 3,
    OS_MEM_NOT_ALIGNED = 4,
    OS_BAD_MUTEX = 5,
    OS_TIMEOUT = 6,
    OS_ERR_IN_ISR = 7,      /* Function cannot be called from ISR */
    OS_ERR_PRIV = 8,        /* Privileged access error */
    OS_NOT_STARTED = 9,     /* OS must be started to call this function, but isn't */
    OS_ENOENT = 10,         /* No such thing */
    OS_EBUSY = 11,          /* Resource busy */
    OS_ERROR = 12,          /* Generic Error */
};

typedef enum os_error os_error_t;

/**
 * @brief Converts an OS error code (`OS_[...]`) to an equivalent system error
 * code (`SYS_E[...]`).
 *
 * @param os_error              The OS error code to convert.
 *
 * @return                      The equivalent system error code.
 */
int os_error_to_sys(os_error_t os_error);

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/apache-mynewt/docs/os/modules/hal/hal_uart/hal_uart.rst`

```
UART
=========

The hardware independent UART interface for Mynewt.

Description
~~~~~~~~~~~

Contains the basic operations to send and receive data over a UART
(Universal Asynchronous Receiver Transmitter). It also includes the API
to apply settings such as speed, parity etc. to the UART. The UART port
should be closed before any reconfiguring.

Examples
~~~~~~~~

This example shows a user writing a character to the uart in blocking
mode where the UART has to block until character has been sent.

.. code-block:: console

    /* write to the console with blocking */
    {
        char *str = "Hello World!";
        char *ptr = str;

        while(*ptr) {
            hal_uart_blocking_tx(MY_UART, *ptr++);
        }
        hal_uart_blocking_tx(MY_UART, '\n');
    }

API
~~~~

.. doxygengroup:: HALUart
    :content-only:
    :members:
```

## Source: `data/rtos/apache-mynewt/docs/os/core_os/cputime/os_cputime.rst`

```
CPU Time
========

The MyNewt ``cputime`` module provides high resolution time and timer
support.

Description
-----------

The ``cputime`` API provides high resolution time and timer support. The
module must be initialized, using the :c:func:`os_cputime_init()` function,
with the clock frequency to use. The module uses the ``hal_timer`` API,
defined in hal/hal_timer.h, to access the hardware timers. It uses the
hardware timer number specified by the ``OS_CPUTIME_TIMER_NUM`` system
configuration setting.

API
-----------------

.. doxygengroup:: OSCPUTime
    :content-only:
    :members:
```
