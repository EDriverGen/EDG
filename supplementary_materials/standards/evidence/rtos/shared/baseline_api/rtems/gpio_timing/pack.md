# Raw RTOS/Bus Pack: rtems / gpio_timing

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/rtems/cpukit/include/rtems.h`

```c
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup RTEMSAPIClassic
 *
 * @brief This header file defines the RTEMS Classic API.
 */

/*
 * Copyright (C) 2020 embedded brains GmbH & Co. KG
 * Copyright (C) 1988, 2008 On-Line Applications Research Corporation (OAR)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file is part of the RTEMS quality process and was automatically
 * generated.  If you find something that needs to be fixed or
 * worded better please post a report or patch to an RTEMS mailing list
 * or raise a bug report:
 *
 * https://www.rtems.org/bugs.html
 *
 * For information on updating and regenerating please refer to the How-To
 * section in the Software Requirements Engineering chapter of the
 * RTEMS Software Engineering manual.  The manual is provided as a part of
 * a release.  For development sources please refer to the online
 * documentation at:
 *
 * https://docs.rtems.org
 */

/* Generated from spec:/rtems/if/header */

#ifndef _RTEMS_H
#define _RTEMS_H

#include <rtems/config.h>
#include <rtems/extension.h>
#include <rtems/fatal.h>
#include <rtems/init.h>
#include <rtems/io.h>
#include <rtems/rtems/barrier.h>
#include <rtems/rtems/cache.h>
#include <rtems/rtems/clock.h>
#include <rtems/rtems/dpmem.h>
#include <rtems/rtems/event.h>
#include <rtems/rtems/intr.h>
#include <rtems/rtems/message.h>
#include <rtems/rtems/object.h>
#include <rtems/rtems/options.h>
#include <rtems/rtems/part.h>
#include <rtems/rtems/ratemon.h>
#include <rtems/rtems/region.h>
#include <rtems/rtems/scheduler.h>
#include <rtems/rtems/sem.h>
#include <rtems/rtems/signal.h>
#include <rtems/rtems/status.h>
#include <rtems/rtems/support.h>
#include <rtems/rtems/tasks.h>
#include <rtems/rtems/timer.h>
#include <rtems/rtems/types.h>

#if defined( RTEMS_MULTIPROCESSING )
  #include <rtems/rtems/mp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Generated from spec:/rtems/if/group */

/**
 * @defgroup RTEMSAPIClassic Classic
 *
 * @ingroup RTEMSAPI
 *
 * @brief This group contains the Classic API managers.
 */

#ifdef __cplusplus
}
#endif

#endif /* _RTEMS_H */
```

## Source: `data/rtos/rtems/cpukit/include/rtems/rtems/clock.h`

```c

/* excerpt lines 1-151 */
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup RTEMSImplClassicClock
 *
 * @brief This header file defines the Clock Manager API.
 */

/*
 * Copyright (C) 2014, 2021 embedded brains GmbH & Co. KG
 * Copyright (C) 1988, 2008 On-Line Applications Research Corporation (OAR)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file is part of the RTEMS quality process and was automatically
 * generated.  If you find something that needs to be fixed or
 * worded better please post a report or patch to an RTEMS mailing list
 * or raise a bug report:
 *
 * https://www.rtems.org/bugs.html
 *
 * For information on updating and regenerating please refer to the How-To
 * section in the Software Requirements Engineering chapter of the
 * RTEMS Software Engineering manual.  The manual is provided as a part of
 * a release.  For development sources please refer to the online
 * documentation at:
 *
 * https://docs.rtems.org
 */

/* Generated from spec:/rtems/clock/if/header */

#ifndef _RTEMS_RTEMS_CLOCK_H
#define _RTEMS_RTEMS_CLOCK_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <rtems/config.h>
#include <sys/_timespec.h>
#include <sys/_timeval.h>
#include <rtems/rtems/status.h>
#include <rtems/rtems/types.h>
#include <rtems/score/watchdogticks.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Generated from spec:/rtems/clock/if/group */

/**
 * @defgroup RTEMSAPIClassicClock Clock Manager
 *
 * @ingroup RTEMSAPIClassic
 *
 * @brief The Clock Manager provides support for time of day and other time
 *   related capabilities.
 */

/* Generated from spec:/rtems/clock/if/bintime */

/* Forward declaration */
struct bintime;

/* Generated from spec:/rtems/clock/if/set */

/**
 * @ingroup RTEMSAPIClassicClock
 *
 * @brief Sets the CLOCK_REALTIME to the time of day.
 *
 * @param time_of_day is the time of day to set the clock.
 *
 * @retval ::RTEMS_SUCCESSFUL The requested operation was successful.
 *
 * @retval ::RTEMS_INVALID_ADDRESS The ``time_of_day`` parameter was NULL.
 *
 * @retval ::RTEMS_INVALID_CLOCK The time of day specified by ``time_of_day``
 *   was invalid.
 *
 * @par Notes
 * @parblock
 * The date, time, and ticks specified by ``time_of_day`` are all
 * range-checked, and an error is returned if any one is out of its valid
 * range.
 *
 * RTEMS can represent time points of the CLOCK_REALTIME clock in nanoseconds
 * ranging from 1988-01-01T00:00:00.000000000Z to
 * 2514-05-31T01:53:03.999999999Z.  The future uptime of the system shall be in
 * this range, otherwise the system behaviour is undefined.  Due to
 * implementation constraints, the time of day set by the directive shall be
 * before 2100-01-01:00:00.000000000Z.  The latest valid time of day accepted
 * by the POSIX clock_settime() is 2400-01-01T00:00:00.999999999Z.
 *
 * The specified time is based on the configured clock tick rate, see the @ref
 * CONFIGURE_MICROSECONDS_PER_TICK application configuration option.
 *
 * Setting the time forward will fire all CLOCK_REALTIME timers which are
 * scheduled at a time point before or at the time set by the directive.  This
 * may unblock tasks, which may preempt the calling task. User-provided timer
 * routines will execute in the context of the caller.
 *
 * It is allowed to call this directive from within interrupt context, however,
 * this is not recommended since an arbitrary number of timers may fire.
 *
 * The directive shall be called at least once to enable the service of
 * CLOCK_REALTIME related directives.  If the clock is not set at least once,
 * they may return an error status.
 * @endparblock
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive may change the priority of a task.  This may cause the
 *   calling task to be preempted.
 *
 * - The directive may unblock a task.  This may cause the calling task to be
 *   preempted.
 *
 * - The time of day set by the directive shall be
 *   1988-01-01T00:00:00.000000000Z or later.
 *
 * - The time of day set by the directive shall be before
 *   2100-01-01T00:00:00.000000000Z.

/* excerpt lines 828-933 */
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive will not cause the calling task to be preempted.
 *
 * - The directive requires a Clock Driver.
 * @endparblock
 */
rtems_status_code rtems_clock_get_seconds_since_epoch(
  rtems_interval *seconds_since_rtems_epoch
);

/* Generated from spec:/rtems/clock/if/get-ticks-per-second */

/**
 * @ingroup RTEMSAPIClassicClock
 *
 * @brief Gets the number of clock ticks per second configured for the
 *   application.
 *
 * @return Returns the number of clock ticks per second configured for this
 *   application.
 *
 * @par Notes
 * The number of clock ticks per second is defined indirectly by the @ref
 * CONFIGURE_MICROSECONDS_PER_TICK configuration option.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive will not cause the calling task to be preempted.
 * @endparblock
 */
rtems_interval rtems_clock_get_ticks_per_second( void );

/* Generated from spec:/rtems/clock/if/get-ticks-per-second-macro */
#define rtems_clock_get_ticks_per_second() _Watchdog_Ticks_per_second

/* Generated from spec:/rtems/clock/if/get-ticks-since-boot */

/**
 * @ingroup RTEMSAPIClassicClock
 *
 * @brief Gets the number of clock ticks since some time point during the
 *   system initialization or the last overflow of the clock tick counter.
 *
 * @return Returns the number of clock ticks since some time point during the
 *   system initialization or the last overflow of the clock tick counter.
 *
 * @par Notes
 * With a 1ms clock tick, this counter overflows after 50 days since boot.
 * This is the historical measure of uptime in an RTEMS system.  The newer
 * service rtems_clock_get_uptime() is another and potentially more accurate
 * way of obtaining similar information.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive will not cause the calling task to be preempted.
 * @endparblock
 */
rtems_interval rtems_clock_get_ticks_since_boot( void );

/* Generated from spec:/rtems/clock/if/get-ticks-since-boot-macro */
#define rtems_clock_get_ticks_since_boot() _Watchdog_Ticks_since_boot

/* Generated from spec:/rtems/clock/if/get-uptime */

/**
 * @ingroup RTEMSAPIClassicClock
 *
 * @brief Gets the seconds and nanoseconds elapsed since some time point during
 *   the system initialization using CLOCK_MONOTONIC.
 *
 * @param[out] uptime is the pointer to a struct timespec object.  When the
 *   directive call is successful, the seconds and nanoseconds elapsed since
 *   some time point during the system initialization and some point during the
 *   directive call using CLOCK_MONOTONIC will be stored in this object.
 *
 * @retval ::RTEMS_SUCCESSFUL The requested operation was successful.
 *
 * @retval ::RTEMS_INVALID_ADDRESS The ``uptime`` parameter was NULL.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive will not cause the calling task to be preempted.
 *
 * - The directive requires a Clock Driver.
 * @endparblock
 */
rtems_status_code rtems_clock_get_uptime( struct timespec *uptime );


/* excerpt lines 995-1157 */
 *   system initialization and some point during the directive call using
 *   CLOCK_MONOTONIC.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive will not cause the calling task to be preempted.
 *
 * - The directive requires a Clock Driver.
 * @endparblock
 */
uint64_t rtems_clock_get_uptime_nanoseconds( void );

/* Generated from spec:/rtems/clock/if/tick-later */

/**
 * @ingroup RTEMSAPIClassicClock
 *
 * @brief Gets a clock tick value which is at least delta clock ticks in the
 *   future.
 *
 * @param delta is the delta value in clock ticks.
 *
 * @return Returns a clock tick counter value which is at least ``delta`` clock
 *   ticks in the future.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive will not cause the calling task to be preempted.
 *
 * - The directive requires a Clock Driver.
 * @endparblock
 */
static inline rtems_interval rtems_clock_tick_later( rtems_interval delta )
{
  return _Watchdog_Ticks_since_boot + delta;
}

/* Generated from spec:/rtems/clock/if/tick-later-usec */

/**
 * @ingroup RTEMSAPIClassicClock
 *
 * @brief Gets a clock tick value which is at least delta microseconds in the
 *   future.
 *
 * @param delta_in_usec is the delta value in microseconds.
 *
 * @return Returns a clock tick counter value which is at least
 *   ``delta_in_usec`` microseconds in the future.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive will not cause the calling task to be preempted.
 *
 * - The directive requires a Clock Driver.
 * @endparblock
 */
static inline rtems_interval rtems_clock_tick_later_usec(
  rtems_interval delta_in_usec
)
{
  rtems_interval us_per_tick;

  us_per_tick = rtems_configuration_get_microseconds_per_tick();

  /*
   * Add one additional tick, since we do not know the time to the clock
   * next tick.
   */
  return _Watchdog_Ticks_since_boot + 1 +
         ( delta_in_usec + us_per_tick - 1 ) / us_per_tick;
}

/* Generated from spec:/rtems/clock/if/tick-before */

/**
 * @ingroup RTEMSAPIClassicClock
 *
 * @brief Indicates if the current clock tick counter is before the ticks.
 *
 * @param ticks is the ticks value to check.
 *
 * @return Returns true, if current clock tick counter indicates a time before
 *   the time in ticks, otherwise returns false.
 *
 * @par Notes
 * @parblock
 * This directive can be used to write busy loops with a timeout.
 *
 * @code
 * status busy( void )
 * {
 *   rtems_interval timeout;
 *
 *   timeout = rtems_clock_tick_later_usec( 10000 );
 *
 *   do {
 *     if ( ok() ) {
 *       return success;
 *     }
 *   } while ( rtems_clock_tick_before( timeout ) );
 *
 *   return timeout;
 * }
 * @endcode
 * @endparblock
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within any runtime context.
 *
 * - The directive will not cause the calling task to be preempted.
 *
 * - The directive requires a Clock Driver.
 * @endparblock
 */
static inline bool rtems_clock_tick_before( rtems_interval ticks )
{
  return (int32_t) ( ticks - _Watchdog_Ticks_since_boot ) > 0;
}

/* Generated from spec:/rtems/clock/if/tick */

/**
 * @brief Announces a clock tick.
 *
 * @par Notes
 * The directive is a legacy interface.  It should not be called by
 * applications directly.  A Clock Driver may call this directive.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within interrupt context.
 *
 * - The directive may be called from within device driver initialization
 *   context.
 *
 * - The directive may be called from within task context.
 * @endparblock
 */
rtems_status_code rtems_clock_tick( void );

#ifdef __cplusplus
}
#endif

#endif /* _RTEMS_RTEMS_CLOCK_H */

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/rtems/cpukit/include/rtems/rtems/timer.h`

```c

/* excerpt lines 1-94 */
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup RTEMSImplClassicTimer
 *
 * @brief This header file provides the Timer Manager API.
 */

/*
 * Copyright (C) 2020, 2021 embedded brains GmbH & Co. KG
 * Copyright (C) 1988, 2008 On-Line Applications Research Corporation (OAR)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file is part of the RTEMS quality process and was automatically
 * generated.  If you find something that needs to be fixed or
 * worded better please post a report or patch to an RTEMS mailing list
 * or raise a bug report:
 *
 * https://www.rtems.org/bugs.html
 *
 * For information on updating and regenerating please refer to the How-To
 * section in the Software Requirements Engineering chapter of the
 * RTEMS Software Engineering manual.  The manual is provided as a part of
 * a release.  For development sources please refer to the online
 * documentation at:
 *
 * https://docs.rtems.org
 */

/* Generated from spec:/rtems/timer/if/header */

#ifndef _RTEMS_RTEMS_TIMER_H
#define _RTEMS_RTEMS_TIMER_H

#include <stddef.h>
#include <rtems/rtems/attr.h>
#include <rtems/rtems/status.h>
#include <rtems/rtems/types.h>
#include <rtems/score/watchdogticks.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Generated from spec:/rtems/timer/if/group */

/**
 * @defgroup RTEMSAPIClassicTimer Timer Manager
 *
 * @ingroup RTEMSAPIClassic
 *
 * @brief The Timer Manager provides support for timer facilities.
 */

/* Generated from spec:/rtems/timer/if/class-bit-not-dormant */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief This timer class bit indicates that the timer is not dormant.
 */
#define TIMER_CLASS_BIT_NOT_DORMANT 0x4

/* Generated from spec:/rtems/timer/if/class-bit-on-task */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief This timer class bit indicates that the timer routine executes in a
 *   task context.

/* excerpt lines 106-211 */

/* Generated from spec:/rtems/timer/if/classes */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief The timer class indicates how the timer was most recently fired.
 */
typedef enum {
  /**
   * @brief This timer class indicates that the timer was never in use.
   */
  TIMER_DORMANT,

  /**
   * @brief This timer class indicates that the timer is currently in use as an
   *   interval timer which will fire in the context of the clock tick ISR.
   */
  TIMER_INTERVAL = TIMER_CLASS_BIT_NOT_DORMANT,

  /**
   * @brief This timer class indicates that the timer is currently in use as an
   *   interval timer which will fire in the context of the Timer Server task.
   */
  TIMER_INTERVAL_ON_TASK = TIMER_CLASS_BIT_NOT_DORMANT |
                           TIMER_CLASS_BIT_ON_TASK,

  /**
   * @brief This timer class indicates that the timer is currently in use as an
   *   time of day timer which will fire in the context of the clock tick ISR.
   */
  TIMER_TIME_OF_DAY = TIMER_CLASS_BIT_NOT_DORMANT |
                      TIMER_CLASS_BIT_TIME_OF_DAY,

  /**
   * @brief This timer class indicates that the timer is currently in use as an
   *   time of day timer which will fire in the context of the Timer Server task.
   */
  TIMER_TIME_OF_DAY_ON_TASK = TIMER_CLASS_BIT_NOT_DORMANT |
                              TIMER_CLASS_BIT_TIME_OF_DAY |
                              TIMER_CLASS_BIT_ON_TASK
} Timer_Classes;

/* Generated from spec:/rtems/timer/if/information */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief The structure contains information about a timer.
 */
typedef struct {
  /**
   * @brief The timer class member indicates how the timer was most recently
   *   fired.
   */
  Timer_Classes the_class;

  /**
   * @brief This member indicates the initial requested interval.
   */
  Watchdog_Interval initial;

  /**
   * @brief This member indicates the time the timer was initially scheduled.
   *
   * The time is in clock ticks since the clock driver initialization or the last
   * clock tick counter overflow.
   */
  Watchdog_Interval start_time;

  /**
   * @brief This member indicates the time the timer was scheduled to fire.
   *
   * The time is in clock ticks since the clock driver initialization or the last
   * clock tick counter overflow.
   */
  Watchdog_Interval stop_time;
} rtems_timer_information;

/* Generated from spec:/rtems/timer/if/get-information */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief Gets information about the timer.
 *
 * @param id is the timer identifier.
 *
 * @param[out] the_info is the pointer to an rtems_timer_information object.
 *   When the directive call is successful, the information about the timer
 *   will be stored in this object.
 *
 * This directive returns information about the timer.
 *
 * @retval ::RTEMS_SUCCESSFUL The requested operation was successful.
 *
 * @retval ::RTEMS_INVALID_ADDRESS The ``the_info`` parameter was NULL.
 *
 * @retval ::RTEMS_INVALID_ID There was no timer associated with the identifier
 *   specified by ``id``.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within interrupt context.

/* excerpt lines 443-552 */
 *
 * - Where the object class corresponding to the directive is configured to use
 *   unlimited objects, the directive may free memory to the RTEMS Workspace.
 * @endparblock
 */
rtems_status_code rtems_timer_delete( rtems_id id );

/* Generated from spec:/rtems/timer/if/fire-after */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief Fires the timer after the interval.
 *
 * @param id is the timer identifier.
 *
 * @param ticks is the interval until the routine is fired in clock ticks.
 *
 * @param routine is the routine to schedule.
 *
 * @param user_data is the argument passed to the routine when it is fired.
 *
 * This directive initiates the timer specified by ``id``.  If the timer is
 * running, it is automatically canceled before being initiated.  The timer is
 * scheduled to fire after an interval of clock ticks has passed specified by
 * ``ticks``.  When the timer fires, the timer service routine ``routine`` will
 * be invoked with the argument ``user_data`` in the context of the clock tick
 * ISR.
 *
 * @retval ::RTEMS_SUCCESSFUL The requested operation was successful.
 *
 * @retval ::RTEMS_INVALID_NUMBER The ``ticks`` parameter was 0.
 *
 * @retval ::RTEMS_INVALID_ADDRESS The ``routine`` parameter was NULL.
 *
 * @retval ::RTEMS_INVALID_ID There was no timer associated with the identifier
 *   specified by ``id``.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within interrupt context.
 *
 * - The directive may be called from within device driver initialization
 *   context.
 *
 * - The directive may be called from within task context.
 *
 * - The directive will not cause the calling task to be preempted.
 * @endparblock
 */
rtems_status_code rtems_timer_fire_after(
  rtems_id                          id,
  rtems_interval                    ticks,
  rtems_timer_service_routine_entry routine,
  void                             *user_data
);

/* Generated from spec:/rtems/timer/if/fire-when */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief Fires the timer at the time of day.
 *
 * @param id is the timer identifier.
 *
 * @param wall_time is the time of day when the routine is fired.
 *
 * @param routine is the routine to schedule.
 *
 * @param user_data is the argument passed to the routine when it is fired.
 *
 * This directive initiates the timer specified by ``id``.  If the timer is
 * running, it is automatically canceled before being initiated.  The timer is
 * scheduled to fire at the time of day specified by ``wall_time``.  When the
 * timer fires, the timer service routine ``routine`` will be invoked with the
 * argument ``user_data`` in the context of the clock tick ISR.
 *
 * @retval ::RTEMS_SUCCESSFUL The requested operation was successful.
 *
 * @retval ::RTEMS_NOT_DEFINED The system date and time was not set.
 *
 * @retval ::RTEMS_INVALID_ADDRESS The ``routine`` parameter was NULL.
 *
 * @retval ::RTEMS_INVALID_ADDRESS The ``wall_time`` parameter was NULL.
 *
 * @retval ::RTEMS_INVALID_CLOCK The time of day was invalid.
 *
 * @retval ::RTEMS_INVALID_ID There was no timer associated with the identifier
 *   specified by ``id``.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within interrupt context.
 *
 * - The directive may be called from within device driver initialization
 *   context.
 *
 * - The directive may be called from within task context.
 *
 * - The directive will not cause the calling task to be preempted.
 * @endparblock
 */
rtems_status_code rtems_timer_fire_when(
  rtems_id                          id,
  const rtems_time_of_day          *wall_time,

/* excerpt lines 613-700 */
 */
rtems_status_code rtems_timer_initiate_server(
  rtems_task_priority priority,
  size_t              stack_size,
  rtems_attribute     attribute_set
);

/* Generated from spec:/rtems/timer/if/server-fire-after */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief Fires the timer after the interval using the Timer Server.
 *
 * @param id is the timer identifier.
 *
 * @param ticks is the interval until the routine is fired in clock ticks.
 *
 * @param routine is the routine to schedule.
 *
 * @param user_data is the argument passed to the routine when it is fired.
 *
 * This directive initiates the timer specified by ``id``.  If the timer is
 * running, it is automatically canceled before being initiated.  The timer is
 * scheduled to fire after an interval of clock ticks has passed specified by
 * ``ticks``.  When the timer fires, the timer service routine ``routine`` will
 * be invoked with the argument ``user_data`` in the context of the Timer
 * Server task.
 *
 * @retval ::RTEMS_SUCCESSFUL The requested operation was successful.
 *
 * @retval ::RTEMS_INCORRECT_STATE The Timer Server was not initiated.
 *
 * @retval ::RTEMS_INVALID_NUMBER The ``ticks`` parameter was 0.
 *
 * @retval ::RTEMS_INVALID_ADDRESS The ``routine`` parameter was NULL.
 *
 * @retval ::RTEMS_INVALID_ID There was no timer associated with the identifier
 *   specified by ``id``.
 *
 * @par Constraints
 * @parblock
 * The following constraints apply to this directive:
 *
 * - The directive may be called from within interrupt context.
 *
 * - The directive may be called from within device driver initialization
 *   context.
 *
 * - The directive may be called from within task context.
 *
 * - The directive will not cause the calling task to be preempted.
 * @endparblock
 */
rtems_status_code rtems_timer_server_fire_after(
  rtems_id                          id,
  rtems_interval                    ticks,
  rtems_timer_service_routine_entry routine,
  void                             *user_data
);

/* Generated from spec:/rtems/timer/if/server-fire-when */

/**
 * @ingroup RTEMSAPIClassicTimer
 *
 * @brief Fires the timer at the time of day using the Timer Server.
 *
 * @param id is the timer identifier.
 *
 * @param wall_time is the time of day when the routine is fired.
 *
 * @param routine is the routine to schedule.
 *
 * @param user_data is the argument passed to the routine when it is fired.
 *
 * This directive initiates the timer specified by ``id``.  If the timer is
 * running, it is automatically canceled before being initiated.  The timer is
 * scheduled to fire at the time of day specified by ``wall_time``.  When the
 * timer fires, the timer service routine ``routine`` will be invoked with the
 * argument ``user_data`` in the context of the Timer Server task.
 *
 * @retval ::RTEMS_SUCCESSFUL The requested operation was successful.
 *
 * @retval ::RTEMS_INCORRECT_STATE The Timer Server was not initiated.
 *
 * @retval ::RTEMS_NOT_DEFINED The system date and time was not set.
 *

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/rtems/cpukit/include/rtems/btimer.h`

```c
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @brief RTEMS Benchmark Timer API for all Boards
 */

/*
 *  COPYRIGHT (c) 2011 Ralf Corsepius Ulm/Germany
 *
 *  Derived from libcsupport/include/timerdrv.h:
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * All the functions declared as extern after this comment
 * MUST be implemented in each BSP.
 */

#ifndef _RTEMS_BTIMER_H
#define _RTEMS_BTIMER_H

#include <stdbool.h>
#include <stdint.h>
#include <rtems/rtems/status.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BenchmarkTimer Benchmark Timer Driver Interface
 *
 * @ingroup RTEMSLegacyBenchmarkDrivers
 *
 * This module defines the interface for the Benchmark Timer Driver.
 *
 * The following methods in this module must be provided by each BSP:
 *
 *   - benchmark_timer_initialize
 *   - benchmark_timer_read
 *   - benchmark_timer_disable_subtracting_average_overhead
 *
 * The units measured are BSP specific but should be at the highest
 * granularity possible.
 *
 * The Benchmark Timer may use the same hardware as the Clock Driver.
 * No RTEMS Timing Tests will use both drivers at the same time.
 */

/**
 * @brief This type is used to return a Benchmark Timer value.
 *
 * This type is used to contain benchmark times. The units are BSP specific.
 */
typedef uint32_t benchmark_timer_t;

/**
 * @brief Initialize the Benchmark Timer
 *
 * This method initializes the benchmark timer and resets it to begin
 * counting.
 */
extern void benchmark_timer_initialize( void );

/**
 * @brief Read the Benchmark Timer
 *
 * This method stops the benchmark timer and returns the number of
 * units that have passed since @a benchmark_timer_initialize was invoked.
 *
 * @return This method returns the number of units with the average overhead
 *          removed. If the value is below the minimum trusted value, zero
 *          is returned.
 */
extern benchmark_timer_t benchmark_timer_read( void );

/**
 * @brief Benchmark Timer Empty Function
 *
 * This method is used to determine loop overhead.
 */
extern rtems_status_code benchmark_timer_empty_function( void );

/**
 * @brief Disable Average Overhead Removal from the Benchmark Timer
 *
 * This method places the benchmark timer in a "raw" mode where it
 * returns the actual number of units which have passed between
 * calls to @a benchmark_timer_initialize and @a benchmark_timer_read
 * counting.
 *
 * @param[in] find_flag indicates to enable or disable the mode
 */
extern void benchmark_timer_disable_subtracting_average_overhead(
  bool find_flag
);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/rtems/cpukit/include/rtems/tod.h`

```c
/* SPDX-License-Identifier: GPL-2.0-with-RTEMS-exception */

/**
 *  @file
 *
 *  @ingroup shared_tod
 *
 *  @brief Real Time Clock Time of Day API Definition
 */

/*
 *
 *  Based on MVME162 TOD by:
 *    COPYRIGHT (C) 1997
 *    by Katsutoshi Shibuya - BU Denken Co.,Ltd. - Sapporo - JAPAN
 *    ALL RIGHTS RESERVED
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef TOD_H
#define TOD_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @defgroup shared_tod RTC
 *
 *  @ingroup RTEMSBSPsShared
 *
 *  @brief Set the RTC
 */
int setRealTime(
  const rtems_time_of_day *tod
);

/*
 *  Get the time from the RTC.
 */

void getRealTime(
  rtems_time_of_day *tod
);

/*
 *  Read real time from RTC and set it to RTEMS' clock manager
 */

void setRealTimeToRTEMS(void);

/*
 *  Read time from RTEMS' clock manager and set it to RTC
 */

void setRealTimeFromRTEMS(void);

/*
 *  Return the difference between RTC and RTEMS' clock manager time in minutes.
 *  If the difference is greater than 1 day, this returns 9999.
 */

int checkRealTime(void);

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/rtems/bsps/include/bsp/gpio.h`

```c
/* SPDX-License-Identifier: GPL-2.0+-with-RTEMS-exception */

/**
 * @file
 *
 * @ingroup rtems_gpio
 *
 * @brief RTEMS GPIO API definition.
 */

/*
 *  Copyright (c) 2014-2015 Andre Marques <andre.lousa.marques at gmail.com>
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef LIBBSP_SHARED_GPIO_H
#define LIBBSP_SHARED_GPIO_H

#include <bsp.h>
#include <rtems.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(BSP_GPIO_PIN_COUNT) || !defined(BSP_GPIO_PINS_PER_BANK)
  #error "BSP_GPIO_PIN_COUNT or BSP_GPIO_PINS_PER_BANK is not defined."
#endif

#if BSP_GPIO_PIN_COUNT <= 0 || BSP_GPIO_PINS_PER_BANK <= 0
  #error "Invalid BSP_GPIO_PIN_COUNT or BSP_GPIO_PINS_PER_BANK."
#endif

#if BSP_GPIO_PINS_PER_BANK > 32
  #error "Invalid BSP_GPIO_PINS_PER_BANK. Must be in the range of 1 to 32."
#endif

#define GPIO_LAST_BANK_PINS BSP_GPIO_PIN_COUNT % BSP_GPIO_PINS_PER_BANK

#if GPIO_LAST_BANK_PINS > 0
  #define GPIO_BANK_COUNT (BSP_GPIO_PIN_COUNT / BSP_GPIO_PINS_PER_BANK) + 1
#else
  #define GPIO_BANK_COUNT BSP_GPIO_PIN_COUNT / BSP_GPIO_PINS_PER_BANK
  #undef GPIO_LAST_BANK_PINS
  #define GPIO_LAST_BANK_PINS BSP_GPIO_PINS_PER_BANK
#endif

#if defined(BSP_GPIO_PINS_PER_SELECT_BANK) && BSP_GPIO_PINS_PER_SELECT_BANK > 32
  #error "Invalid BSP_GPIO_PINS_PER_SELECT_BANK. Must under and including 32."
#elif defined(BSP_GPIO_PINS_PER_SELECT_BANK) <= 32
  #define GPIO_SELECT_BANK_COUNT \
    BSP_GPIO_PINS_PER_BANK / BSP_GPIO_PINS_PER_SELECT_BANK
#endif

#define INTERRUPT_SERVER_PRIORITY 1
#define INTERRUPT_SERVER_STACK_SIZE 2 * RTEMS_MINIMUM_STACK_SIZE
#define INTERRUPT_SERVER_MODES RTEMS_TIMESLICE | RTEMS_PREEMPT
#define INTERRUPT_SERVER_ATTRIBUTES RTEMS_DEFAULT_ATTRIBUTES

#define GPIO_INPUT_ERROR ~0

/**
 * @name GPIO data structures
 *
 * @{
 */

/**
 * @brief The set of possible configurations for a GPIO pull-up resistor.
 *
 * Enumerated type to define the possible pull-up resistor configurations
 * for a GPIO pin.
 */
typedef enum
{
  PULL_UP = 1,
  PULL_DOWN,
  NO_PULL_RESISTOR
} rtems_gpio_pull_mode;

/**
 * @brief The set of possible functions a pin can have.
 *
 * Enumerated type to define a pin function.
 */
typedef enum
{
  DIGITAL_INPUT = 0,
  DIGITAL_OUTPUT,
  BSP_SPECIFIC,
  NOT_USED
} rtems_gpio_function;

/**
 * @brief The set of possible interrupts a GPIO pin can generate.
 *
 * Enumerated type to define a GPIO pin interrupt.
 */
typedef enum
{
  FALLING_EDGE = 0,
  RISING_EDGE,
  LOW_LEVEL,
  HIGH_LEVEL,
  BOTH_EDGES,
  BOTH_LEVELS,
  NONE
} rtems_gpio_interrupt;

/**
 * @brief The set of possible handled states an user-defined interrupt
 *        handler can return.
 *
 * Enumerated type to define an interrupt handler handled state.
 */
typedef enum
{
  IRQ_HANDLED,
  IRQ_NONE
} rtems_gpio_irq_state;

/**
 * @brief The set of flags to specify an user-defined interrupt handler
 *        uniqueness on a GPIO pin.
 *
 * Enumerated type to define an interrupt handler shared flag.
 */
typedef enum
{
  SHARED_HANDLER,
  UNIQUE_HANDLER
} rtems_gpio_handler_flag;

/**
 * @brief Object containing relevant information for assigning a BSP specific
 *        function to a pin.
 *
 * Encapsulates relevant data for a BSP specific GPIO function.
 */
typedef struct
{
  /* The BSP defined function code. */
  uint32_t io_function;

  void *pin_data;
} rtems_gpio_specific_data;

/**
 * @brief Object containing configuration information
 *        regarding interrupts.
 */
typedef struct
{
  rtems_gpio_interrupt active_interrupt;

  rtems_gpio_handler_flag handler_flag;

  bool threaded_interrupts;

  /* Interrupt handler function. */
  rtems_gpio_irq_state (*handler) (void *arg);

  /* Interrupt handler function arguments. */
  void *arg;

  /* Software switch debounce settings. It should contain the amount of clock
   * ticks that must pass between interrupts to ensure that the interrupt
   * was not caused by a switch bounce.
   * If set to 0 this feature is disabled . */
  uint32_t debounce_clock_tick_interval;
} rtems_gpio_interrupt_configuration;

/**
 * @brief Object containing configuration information
 *        to request/update a GPIO pin.
 */
typedef struct
{
  /* Processor pin number. */
  uint32_t pin_number;
  rtems_gpio_function function;

  /* Pull resistor setting. */
  rtems_gpio_pull_mode pull_mode;

  /* If digital out pin, set to TRUE to set the pin to logical high,
   * or FALSE for logical low. If not a digital out then this
   * is ignored. */
  bool output_enabled;

  /* If true inverts digital in/out applicational logic. */
  bool logic_invert;

  /* Pin interrupt configuration. Should be NULL if not used. */
  rtems_gpio_interrupt_configuration *interrupt;

  /* Structure with BSP specific data, to use during the pin request.
   * If function == BSP_SPECIFIC this should have a pointer to
   * a rtems_gpio_specific_data structure.
   *
   * If not this field may be NULL. This is passed to the BSP function
   * so any BSP specific data can be passed to it through this pointer. */
  void *bsp_specific;
} rtems_gpio_pin_conf;

/**
 * @brief Object containing configuration information
 *        to assign GPIO functions to multiple pins
 *        at the same time. To be used by BSP code only.
 */
typedef struct
{
  /* Global GPIO pin number. */
  uint32_t pin_number;

  /* RTEMS GPIO pin function code. */
  rtems_gpio_function function;

  /* BSP specific function code. Only used if function == BSP_SPECIFIC */
  uint32_t io_function;

  /* BSP specific data. */
  void *bsp_specific;
} rtems_gpio_multiple_pin_select;

/**
 * @brief Object containing configuration information
 *        to request a GPIO pin group.
 */
typedef struct
{
  const rtems_gpio_pin_conf *digital_inputs;
  uint32_t input_count;

  const rtems_gpio_pin_conf *digital_outputs;
  uint32_t output_count;

  const rtems_gpio_pin_conf *bsp_specifics;
  uint32_t bsp_specific_pin_count;
} rtems_gpio_group_definition;

/**
 * @brief Opaque type for a GPIO pin group.
 */
typedef struct rtems_gpio_group rtems_gpio_group;

/** @} */

/**
 * @name gpio Usage
 *
 * @{
 */

/**
 * @brief Initializes the GPIO API.
 *
 * @retval RTEMS_SUCCESSFUL API successfully initialized.
 * @retval * @see rtems_semaphore_create().
 */
extern rtems_status_code rtems_gpio_initialize(void);

/**
 * @brief Instantiates a GPIO pin group.
 *        To define the group @see rtems_gpio_define_pin_group().
 *
 * @retval rtems_gpio_group pointer.
 */
extern rtems_gpio_group *rtems_gpio_create_pin_group(void);

/**
 * @brief Requests a GPIO pin group configuration.
 *
 * @param[in] group_definition rtems_gpio_group_definition structure filled with
 *                             the group pins configurations.
 * @param[out] group Reference to the created group.
 *
 * @retval RTEMS_SUCCESSFUL Pin group was configured successfully.
 * @retval RTEMS_UNSATISFIED @var group_definition or @var group is NULL,
 *                           the @var pins are not from the same bank,
 *                           no pins were defined or could not satisfy at
 *                           least one given configuration.
 * @retval RTEMS_RESOURCE_IN_USE At least one pin is already being used.
 * @retval * @see rtems_semaphore_create().
 */
extern rtems_status_code rtems_gpio_define_pin_group(
  const rtems_gpio_group_definition *group_definition,
  rtems_gpio_group *group
);

/**
 * @brief Writes a value to the group's digital outputs. The pins order
 *        is as defined in the group definition.
 *
 * @param[in] data Data to write/send.
 * @param[in] group Reference to the group.
 *
 * @retval RTEMS_SUCCESSFUL Data successfully written.
 * @retval RTEMS_NOT_DEFINED Group has no output pins.
 * @retval RTEMS_UNSATISFIED Could not operate on at least one of the pins.
 */
extern rtems_status_code rtems_gpio_write_group(
  uint32_t data,
  rtems_gpio_group *group
);

/**
 * @brief Reads the value/level of the group's digital inputs. The pins order
 *        is as defined in the group definition.
 *
 * @param[in] group Reference to the group.
 *
 * @retval The function returns a 32-bit bitmask with the group's input pins
 *         current logical values.
 * @retval GPIO_INPUT_ERROR Group has no input pins.
 */
extern uint32_t rtems_gpio_read_group(rtems_gpio_group *group);

/**
 * @brief Performs a BSP specific operation on a group of pins. The pins order
 *        is as defined in the group definition.
 *
 * @param[in] group Reference to the group.
 * @param[in] arg Pointer to a BSP defined structure with BSP-specific
 *                data. This field is handled by the BSP.
 *
 * @retval RTEMS_SUCCESSFUL Operation completed with success.
 * @retval RTEMS_NOT_DEFINED Group has no BSP specific pins, or the BSP does not
 *                           support BSP specific operations for groups.
 * @retval RTEMS_UNSATISFIED Could not operate on at least one of the pins.
 */
extern rtems_status_code rtems_gpio_group_bsp_specific_operation(
  rtems_gpio_group *group,
  void *arg
);

/**
 * @brief Requests a GPIO pin configuration.
 *
 * @param[in] conf rtems_gpio_pin_conf structure filled with the pin information
 *                 and desired configurations.
 *
 * @retval RTEMS_SUCCESSFUL Pin was configured successfully.
 * @retval RTEMS_UNSATISFIED Could not satisfy the given configuration.
 */
extern rtems_status_code rtems_gpio_request_configuration(
  const rtems_gpio_pin_conf *conf
);

/**
 * @brief Updates the current configuration of a GPIO pin .
 *
 * @param[in] conf rtems_gpio_pin_conf structure filled with the pin information
 *                 and desired configurations.
 *
 * @retval RTEMS_SUCCESSFUL Pin configuration was updated successfully.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED The pin is not being used.
 * @retval RTEMS_UNSATISFIED Could not update the pin's configuration.
 */
extern rtems_status_code rtems_gpio_update_configuration(
  const rtems_gpio_pin_conf *conf
);

/**
 * @brief Sets multiple output GPIO pins with the logical high.
 *
 * @param[in] pin_numbers Array with the GPIO pin numbers to set.
 * @param[in] count Number of GPIO pins to set.
 *
 * @retval RTEMS_SUCCESSFUL All pins were set successfully.
 * @retval RTEMS_INVALID_ID At least one pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED At least one of the received pins
 *                              is not configured as a digital output.
 * @retval RTEMS_UNSATISFIED Could not set the GPIO pins.
 */
extern rtems_status_code rtems_gpio_multi_set(
  uint32_t *pin_numbers,
  uint32_t pin_count
);

/**
 * @brief Sets multiple output GPIO pins with the logical low.
 *
 * @param[in] pin_numbers Array with the GPIO pin numbers to clear.
 * @param[in] count Number of GPIO pins to clear.
 *
 * @retval RTEMS_SUCCESSFUL All pins were cleared successfully.
 * @retval RTEMS_INVALID_ID At least one pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED At least one of the received pins
 *                              is not configured as a digital output.
 * @retval RTEMS_UNSATISFIED Could not clear the GPIO pins.
 */
extern rtems_status_code rtems_gpio_multi_clear(
  uint32_t *pin_numbers,
  uint32_t pin_count
);

/**
 * @brief Returns the value (level) of multiple GPIO input pins.
 *
 * @param[in] pin_numbers Array with the GPIO pin numbers to read.
 * @param[in] count Number of GPIO pins to read.
 *
 * @retval Bitmask with the values of the corresponding pins.
 *         0 for logical low and 1 for logical high.
 * @retval GPIO_INPUT_ERROR Could not read at least one pin level.
 */
extern uint32_t rtems_gpio_multi_read(
  uint32_t *pin_numbers,
  uint32_t pin_count
);

/**
 * @brief Sets an output GPIO pin with the logical high.
 *
 * @param[in] pin_number GPIO pin number.
 *
 * @retval RTEMS_SUCCESSFUL Pin was set successfully.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED The received pin is not configured
 *                              as a digital output.
 * @retval RTEMS_UNSATISFIED Could not set the GPIO pin.
 */
extern rtems_status_code rtems_gpio_set(uint32_t pin_number);

/**
 * @brief Sets an output GPIO pin with the logical low.
 *
 * @param[in] pin_number GPIO pin number.
 *
 * @retval RTEMS_SUCCESSFUL Pin was cleared successfully.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED The received pin is not configured
 *                              as a digital output.
 * @retval RTEMS_UNSATISFIED Could not clear the GPIO pin.
 */
extern rtems_status_code rtems_gpio_clear(uint32_t pin_number);

/**
 * @brief Returns the value (level) of a GPIO input pin.
 *
 * @param[in] pin_number GPIO pin number.
 *
 * @retval The function returns 0 or 1 depending on the pin current
 *         logical value.
 * @retval -1 Pin number is invalid, or not a digital input pin.
 */
extern int rtems_gpio_get_value(uint32_t pin_number);

/**
 * @brief Requests multiple GPIO pin configurations. If the BSP provides
 *        support for parallel selection each call to this function will
 *        result in a single call to the GPIO hardware, else each pin
 *        configuration will be done in individual and sequential calls.
 *        All pins must belong to the same GPIO bank.
 *
 * @param[in] pins Array of rtems_gpio_pin_conf structures filled with the pins
 *                 information and desired configurations. All pins must belong
 *                 to the same GPIO bank.
 * @param[in] pin_count Number of pin configurations in the @var pins array.
 *
 * @retval RTEMS_SUCCESSFUL All pins were configured successfully.
 * @retval RTEMS_INVALID_ID At least one pin number in the @var pins array
 *                          is invalid.
 * @retval RTEMS_RESOURCE_IN_USE At least one pin is already being used.
 * @retval RTEMS_UNSATISFIED Could not satisfy at least one given configuration.
 */
extern rtems_status_code rtems_gpio_multi_select(
  const rtems_gpio_pin_conf *pins,
  uint8_t pin_count
);

/**
 * @brief Assigns a certain function to a GPIO pin.
 *
 * @param[in] pin_number GPIO pin number.
 * @param[in] function The new function for the pin.
 * @param[in] output_enabled If TRUE and @var function is DIGITAL_OUTPUT,
 *                           then the pin is set with the logical high.
 *                           Otherwise it is set with logical low.
 * @param[in] logic_invert Reverses the digital I/O logic for DIGITAL_INPUT
 *                         and DIGITAL_OUTPUT pins.
 * @param[in] bsp_specific Pointer to a BSP defined structure with BSP-specific
 *                         data. This field is handled by the BSP.
 *
 * @retval RTEMS_SUCCESSFUL Pin was configured successfully.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_RESOURCE_IN_USE The received pin is already being used.
 * @retval RTEMS_UNSATISFIED Could not assign the GPIO function.
 * @retval RTEMS_NOT_DEFINED GPIO function not defined, or NOT_USED.
 */
extern rtems_status_code rtems_gpio_request_pin(
  uint32_t pin_number,
  rtems_gpio_function function,
  bool output_enable,
  bool logic_invert,
  void *bsp_specific
);

/**
 * @brief Configures a single GPIO pin pull resistor.
 *
 * @param[in] pin_number GPIO pin number.
 * @param[in] mode The pull resistor mode.
 *
 * @retval RTEMS_SUCCESSFUL Pull resistor successfully configured.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_UNSATISFIED Could not set the pull mode.
 */
extern rtems_status_code rtems_gpio_resistor_mode(
  uint32_t pin_number,
  rtems_gpio_pull_mode mode
);

/**
 * @brief Releases a GPIO pin, making it available to be used again.
 *
 * @param[in] pin_number GPIO pin number.
 *
 * @retval RTEMS_SUCCESSFUL Pin successfully disabled.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval * Could not disable an active interrupt on this pin,
 *           @see rtems_gpio_disable_interrupt().
 */
extern rtems_status_code rtems_gpio_release_pin(uint32_t pin_number);

/**
 * @brief Releases a GPIO pin, making it available to be used again.
 *
 * @param[in] conf GPIO pin configuration to be released.
 *
 * @retval RTEMS_SUCCESSFUL Pin successfully disabled.
 * @retval RTEMS_UNSATISFIED Pin configuration is NULL.
 * @retval * @see rtems_gpio_release_pin().
 */
extern rtems_status_code rtems_gpio_release_configuration(
  const rtems_gpio_pin_conf *conf
);

/**
 * @brief Releases multiple GPIO pins, making them available to be used again.
 *
 * @param[in] pins Array of rtems_gpio_pin_conf structures.
 * @param[in] pin_count Number of pin configurations in the @var pins array.
 *
 * @retval RTEMS_SUCCESSFUL Pins successfully disabled.
 * @retval RTEMS_UNSATISFIED @var pins array is NULL.
 * @retval * @see rtems_gpio_release_pin().
 */
extern rtems_status_code rtems_gpio_release_multiple_pins(
  const rtems_gpio_pin_conf *pins,
  uint32_t pin_count
);

/**
 * @brief Releases a GPIO pin group, making the pins used available to be
 *        repurposed.
 *
 * @param[in] conf GPIO pin configuration to be released.
 *
 * @retval RTEMS_SUCCESSFUL Pins successfully disabled.
 * @retval * @see rtems_gpio_release_pin(), @see rtems_semaphore_delete() or
 *           @see rtems_semaphore_flush().
 */
extern rtems_status_code rtems_gpio_release_pin_group(
  rtems_gpio_group *group
);

/**
 * @brief Attaches a debouncing function to a given pin/switch.
 *        Debouncing is done by requiring a certain number of clock ticks to
 *        pass between interrupts. Any interrupt fired too close to the last
 *        will be ignored as it is probably the result of an involuntary
 *        switch/button bounce after being released.
 *
 * @param[in] pin_number GPIO pin number.
 * @param[in] ticks Minimum number of clock ticks that must pass between
 *                  interrupts so it can be considered a legitimate
 *                  interrupt.
 *
 * @retval RTEMS_SUCCESSFUL Debounce function successfully attached to the pin.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED The current pin is not configured as a digital
 *                              input, hence it can not be connected to a switch,
 *                              or interrupts are not enabled for this pin.
 */
extern rtems_status_code rtems_gpio_debounce_switch(
  uint32_t pin_number,
  int ticks
);

/**
 * @brief Connects a new user-defined interrupt handler to a given pin.
 *
 * @param[in] pin_number GPIO pin number.
 * @param[in] handler Pointer to a function that will be called every time
 *                    the enabled interrupt for the given pin is generated.
 *                    This function must return information about its
 *                    handled/unhandled state.
 * @param[in] arg Void pointer to the arguments of the user-defined handler.
 *
 * @retval RTEMS_SUCCESSFUL Handler successfully connected to this pin.
 * @retval RTEMS_NO_MEMORY Could not connect more user-defined handlers to
 *                         the given pin.
 * @retval RTEMS_NOT_CONFIGURED The given pin has no interrupt configured.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_TOO_MANY The pin's current handler is set as unique.
 * @retval RTEMS_RESOURCE_IN_USE The current user-defined handler for this pin
 *                               is unique.
 */
extern rtems_status_code rtems_gpio_interrupt_handler_install(
  uint32_t pin_number,
  rtems_gpio_irq_state (*handler) (void *arg),
  void *arg
);

/**
 * @brief Enables interrupts to be generated on a given GPIO pin.
 *        When fired that interrupt will call the given handler.
 *
 * @param[in] pin_number GPIO pin number.
 * @param[in] interrupt Type of interrupt to enable for the pin.
 * @param[in] flag Defines the uniqueness of the interrupt handler for the pin.
 * @param[in] threaded_handling Defines if the handler should be called from a
 *                              thread/task or from normal ISR contex.
 * @param[in] handler Pointer to a function that will be called every time
 *                    @var interrupt is generated. This function must return
 *                    information about its handled/unhandled state.
 * @param[in] arg Void pointer to the arguments of the user-defined handler.
 *
 * @retval RTEMS_SUCCESSFUL Interrupt successfully enabled for this pin.
 * @retval RTEMS_UNSATISFIED Could not install the GPIO ISR, create/start
 *                           the handler task, or enable the interrupt
 *                           on the pin.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED The received pin is not configured
 *                              as a digital input, the pin is on a
 *                              pin grouping.
 * @retval RTEMS_RESOURCE_IN_USE The pin already has an enabled interrupt,
 *                               or the handler threading policy does not match
 *                               the bank's policy.
 * @retval RTEMS_NO_MEMORY Could not store the pin's interrupt configuration.
 */
extern rtems_status_code rtems_gpio_enable_interrupt(
  uint32_t pin_number,
  rtems_gpio_interrupt interrupt,
  rtems_gpio_handler_flag flag,
  bool threaded_handling,
  rtems_gpio_irq_state (*handler) (void *arg),
  void *arg
);

/**
 * @brief Disconnects an user-defined interrupt handler from the given pin.
 *        If in the end there are no more user-defined handlers connected
 *        to the pin, interrupts are disabled on the given pin.
 *
 * @param[in] pin_number GPIO pin number.
 * @param[in] handler Pointer to the user-defined handler
 * @param[in] arg Void pointer to the arguments of the user-defined handler.
 *
 * @retval RTEMS_SUCCESSFUL Handler successfully disconnected from this pin.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED Pin has no active interrupts.
 * @retval * @see rtems_gpio_disable_interrupt()
 */
extern rtems_status_code rtems_gpio_interrupt_handler_remove(
  uint32_t pin_number,
  rtems_gpio_irq_state (*handler) (void *arg),
  void *arg
);

/**
 * @brief Stops interrupts from being generated on a given GPIO pin
 *        and removes the corresponding handler.
 *
 * @param[in] pin_number GPIO pin number.
 *
 * @retval RTEMS_SUCCESSFUL Interrupt successfully disabled for this pin.
 * @retval RTEMS_INVALID_ID Pin number is invalid.
 * @retval RTEMS_NOT_CONFIGURED Pin has no active interrupts.
 * @retval RTEMS_UNSATISFIED Could not remove the current interrupt handler,
 *                           could not recognize the current active interrupt
 *                           on this pin or could not disable interrupts on
 *                           this pin.
 */
extern rtems_status_code rtems_gpio_disable_interrupt(uint32_t pin_number);

/**
 * @brief Sets multiple output GPIO pins with the logical high.
 *        This must be implemented by each BSP.
 *
 * @param[in] bank GPIO bank number.
 * @param[in] bitmask Bitmask of GPIO pins to set in the given bank.
 *
 * @retval RTEMS_SUCCESSFUL All pins were set successfully.
 * @retval RTEMS_UNSATISFIED Could not set at least one of the pins.
 */
extern rtems_status_code rtems_gpio_bsp_multi_set(
  uint32_t bank,
  uint32_t bitmask
);

/**
 * @brief Sets multiple output GPIO pins with the logical low.
 *        This must be implemented by each BSP.
 *
 * @param[in] bank GPIO bank number.
 * @param[in] bitmask Bitmask of GPIO pins to clear in the given bank.
 *
 * @retval RTEMS_SUCCESSFUL All pins were cleared successfully.
 * @retval RTEMS_UNSATISFIED Could not clear at least one of the pins.
 */
extern rtems_status_code rtems_gpio_bsp_multi_clear(
  uint32_t bank,
  uint32_t bitmask
);

/**
 * @brief Returns the value (level) of multiple GPIO input pins.
 *        This must be implemented by each BSP.
 *
 * @param[in] bank GPIO bank number.
 * @param[in] bitmask Bitmask of GPIO pins to read in the given bank.
 *
/* ... truncated ... */
```

## Source: `data/rtos/rtems/bsps/shared/dev/gpio/gpio-support.c`

```c
/* SPDX-License-Identifier: GPL-2.0+-with-RTEMS-exception */

/**
 * @file
 *
 * @ingroup rtems_gpio
 *
 * @brief RTEMS GPIO API implementation.
 */

/*
 *  Copyright (c) 2014-2015 Andre Marques <andre.lousa.marques at gmail.com>
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#include <rtems/score/atomic.h>
#include <rtems/chain.h>
#include <bsp/irq-generic.h>
#include <bsp/gpio.h>
#include <rtems/score/assert.h>
#include <stdlib.h>

/**
 * @brief GPIO API mutex attributes.
 */
#define MUTEX_ATTRIBUTES (     \
  RTEMS_LOCAL                  \
  | RTEMS_PRIORITY             \
  | RTEMS_BINARY_SEMAPHORE     \
  | RTEMS_INHERIT_PRIORITY     \
  | RTEMS_NO_PRIORITY_CEILING  \
)

#define CREATE_LOCK(name, lock_id) rtems_semaphore_create(   \
  name,                                                      \
  1,                                                         \
  MUTEX_ATTRIBUTES,                                          \
  0,                                                         \
  lock_id                                                    \
)

#define ACQUIRE_LOCK(_m) \
  do { \
    rtems_status_code _sc; \
    _sc = rtems_semaphore_obtain(_m, RTEMS_WAIT, RTEMS_NO_TIMEOUT); \
    _Assert( _sc == RTEMS_SUCCESSFUL ); \
    (void) _sc; \
  } while (0)

#define RELEASE_LOCK(_m) \
  do { \
    rtems_status_code _sc; \
    _sc = rtems_semaphore_release(_m); \
    _Assert( _sc == RTEMS_SUCCESSFUL ); \
    (void) _sc; \
  } while (0)

/**
 * @brief Object containing relevant information about a GPIO group.
 *
 * Encapsulates relevant data for a GPIO pin group.
 */
struct rtems_gpio_group
{
  rtems_chain_node node;

  uint32_t *digital_inputs;
  uint32_t digital_input_bank;
  uint32_t input_count;

  uint32_t *digital_outputs;
  uint32_t digital_output_bank;
  uint32_t output_count;

  uint32_t *bsp_speficifc_pins;
  uint32_t bsp_specific_bank;
  uint32_t bsp_specific_pin_count;

  rtems_id group_lock;
};

/**
 * @brief Object containing relevant information to a list of user-defined
 *        interrupt handlers.
 *
 * Encapsulates relevant data for a GPIO interrupt handler.
 */
typedef struct
{
  rtems_chain_node node;

  /* User-defined ISR routine. */
  rtems_gpio_irq_state (*handler) (void *arg);

  /* User-defined arguments for the ISR routine. */
  void *arg;
} gpio_handler_node;

/**
 * @brief Object containing relevant information of a pin's interrupt
 *        configuration/state.
 *
 * Encapsulates relevant data of a GPIO pin interrupt state.
 */
typedef struct
{
  /* Currently active interrupt. */
  rtems_gpio_interrupt active_interrupt;

  /* ISR shared flag. */
  rtems_gpio_handler_flag handler_flag;

  /* Linked list of interrupt handlers. */
  rtems_chain_control handler_chain;

  /* Switch-deboucing information. */
  uint32_t debouncing_tick_count;
  rtems_interval last_isr_tick;
} gpio_pin_interrupt_state;

/**
 * @brief Object containing information on a GPIO pin.
 *
 * Encapsulates relevant data about a GPIO pin.
 */
typedef struct
{
  rtems_gpio_function pin_function;

  /* GPIO pull resistor configuration. */
  rtems_gpio_pull_mode resistor_mode;

  /* If true inverts digital in/out applicational logic. */
  bool logic_invert;

  /* True if the pin is on a group. */
  bool on_group;

  /* Interrupt data for a pin. This field is NULL if no interrupt is enabled
   * on the pin. */
  gpio_pin_interrupt_state *interrupt_state;
} gpio_pin;

/**
 * @brief Object containing relevant information regarding a GPIO bank state.
 *
 * Encapsulates relevant data for a GPIO bank.
 */
typedef struct
{
  uint32_t bank_number;
  uint32_t interrupt_counter;
  rtems_id lock;

  /* If TRUE the interrupts on the bank will be called
   * by a rtems interrupt server, otherwise they will be handled
   * in the normal ISR context. */
  bool threaded_interrupts;
} gpio_bank;

static gpio_pin gpio_pin_state[BSP_GPIO_PIN_COUNT];
static Atomic_Flag init_flag = ATOMIC_INITIALIZER_FLAG;
static gpio_bank gpio_bank_state[GPIO_BANK_COUNT];
static Atomic_Uint threaded_interrupt_counter = ATOMIC_INITIALIZER_UINT(0);
static rtems_chain_control gpio_group;

#define BANK_NUMBER(pin_number) pin_number / BSP_GPIO_PINS_PER_BANK
#define PIN_NUMBER(pin_number) pin_number % BSP_GPIO_PINS_PER_BANK

static int debounce_switch(gpio_pin_interrupt_state *interrupt_state)
{
  rtems_interval time;

  time = rtems_clock_get_ticks_since_boot();

  /* If not enough time has elapsed since last interrupt. */
  if (
      (time - interrupt_state->last_isr_tick) <
      interrupt_state->debouncing_tick_count
  ) {
    return -1;
  }

  interrupt_state->last_isr_tick = time;

  return 0;
}

/* Returns the amount of pins in a bank. */
static uint32_t get_bank_pin_count(uint32_t bank)
{
  /* If the current bank is the last bank, which may not be completely filled. */
  if ( bank == GPIO_BANK_COUNT - 1 ) {
    return GPIO_LAST_BANK_PINS;
  }

  return BSP_GPIO_PINS_PER_BANK;
}

/* GPIO generic bank ISR. This may be called directly as response to an
 * interrupt, or by the rtems interrupt server task if the GPIO bank
 * uses threading interrupt handling. */
static void generic_bank_isr(void *arg)
{
  gpio_pin_interrupt_state *interrupt_state;
  rtems_chain_control *handler_list;
  rtems_chain_node *node;
  rtems_chain_node *next_node;
  gpio_handler_node *isr_node;
  rtems_vector_number vector;
  uint32_t event_status;
  uint32_t bank_number;
  uint32_t bank_start_pin;
  uint8_t handled_count;
  int rv;
  uint8_t i;

  bank_number = *((uint32_t*) arg);

  assert ( bank_number < GPIO_BANK_COUNT );

  /* Calculate bank start address in the pin_state array. */
  bank_start_pin = bank_number * BSP_GPIO_PINS_PER_BANK;

  vector = rtems_gpio_bsp_get_vector(bank_number);

  /* If this bank does not use threaded interrupts we have to
   * disable the vector. Otherwise the interrupt server does it. */
  if ( gpio_bank_state[bank_number].threaded_interrupts == false ) {
    /* Prevents more interrupts from being generated on GPIO. */
    bsp_interrupt_vector_disable(vector);
  }

  /* Obtains a 32-bit bitmask, with the pins currently reporting interrupts
   * signaled with 1. */
  event_status = rtems_gpio_bsp_interrupt_line(vector);

  /* Iterates through the bitmask and calls the corresponding handler
   * for active interrupts. */
  for ( i = 0; i < get_bank_pin_count(bank_number); ++i ) {
    /* If active, wake the corresponding pin's ISR task. */
    if ( event_status & (1 << i) ) {
      interrupt_state = gpio_pin_state[bank_start_pin + i].interrupt_state;

      assert ( interrupt_state != NULL );

      handled_count = 0;

      if ( gpio_bank_state[bank_number].threaded_interrupts ) {
        ACQUIRE_LOCK(gpio_bank_state[bank_number].lock);
      }

      /* If this pin has the debouncing function attached, call it. */
      if ( interrupt_state->debouncing_tick_count > 0 ) {
        rv = debounce_switch(interrupt_state);

        /* If the handler call was caused by a switch bounce,
         * ignores and move on. */
        if ( rv < 0 ) {
          if ( gpio_bank_state[bank_number].threaded_interrupts ) {
            RELEASE_LOCK(gpio_bank_state[bank_number].lock);
          }

          continue;
        }
      }

      handler_list = &interrupt_state->handler_chain;

      node = rtems_chain_first(handler_list);

      /* Iterate the ISR list. */
      while ( !rtems_chain_is_tail(handler_list, node) ) {
        isr_node = (gpio_handler_node *) node;

        next_node = node->next;

        if ( (isr_node->handler)(isr_node->arg) == IRQ_HANDLED ) {
          ++handled_count;
        }

        node = next_node;
      }

      /* If no handler assumed the interrupt,
       * treat it as a spurious interrupt. */
      if ( handled_count == 0 ) {
        bsp_interrupt_handler_default(rtems_gpio_bsp_get_vector(bank_number));
      }

      if ( gpio_bank_state[bank_number].threaded_interrupts ) {
        RELEASE_LOCK(gpio_bank_state[bank_number].lock);
      }
    }
  }

  if ( gpio_bank_state[bank_number].threaded_interrupts == false ) {
    bsp_interrupt_vector_enable(vector);
  }
}

/* Verifies if all pins in the received pin array are from the same bank and
 * have the defined GPIO function. Produces bitmask of the received pins. */
static rtems_status_code get_pin_bitmask(
  uint32_t *pins,
  uint32_t pin_count,
  uint32_t *bank_number,
  uint32_t *bitmask,
  rtems_gpio_function function
) {
  uint32_t pin_number;
  uint32_t bank = 0;
  uint8_t i;

  if ( pin_count < 1 ) {
    return RTEMS_UNSATISFIED;
  }

  *bitmask = 0;

  for ( i = 0; i < pin_count; ++i ) {
    pin_number = pins[i];

    if ( pin_number >= BSP_GPIO_PIN_COUNT ) {
      return RTEMS_INVALID_ID;
    }

    if ( i == 0 ) {
      bank = BANK_NUMBER(pin_number);
      *bank_number = bank;

      ACQUIRE_LOCK(gpio_bank_state[bank].lock);
    }
    else if ( bank != BANK_NUMBER(pin_number) ) {
      RELEASE_LOCK(gpio_bank_state[bank].lock);

      return RTEMS_UNSATISFIED;
    }

    if (
        gpio_pin_state[pin_number].pin_function != function ||
        gpio_pin_state[pin_number].on_group
    ) {
      RELEASE_LOCK(gpio_bank_state[bank].lock);

      return RTEMS_NOT_CONFIGURED;
    }

    *bitmask |= (1 << PIN_NUMBER(pin_number));
  }

  RELEASE_LOCK(gpio_bank_state[bank].lock);

  return RTEMS_SUCCESSFUL;
}

static rtems_status_code check_same_bank_and_availability(
  const rtems_gpio_pin_conf *pin_confs,
  uint32_t pin_count,
  uint32_t *bank_number,
  uint32_t *pins
) {
  uint32_t pin_number;
  uint32_t bank;
  uint8_t i;

  for ( i = 0; i < pin_count; ++i ) {
    pin_number = pin_confs[i].pin_number;

    bank = BANK_NUMBER(pin_number);

    if ( i == 0 ) {
      *bank_number = bank;

      ACQUIRE_LOCK(gpio_bank_state[bank].lock);
    }
    else if ( bank != *bank_number ) {
      RELEASE_LOCK(gpio_bank_state[*bank_number].lock);

      return RTEMS_UNSATISFIED;
    }

    if ( gpio_pin_state[pin_number].pin_function != NOT_USED ) {
      RELEASE_LOCK(gpio_bank_state[bank].lock);

      return RTEMS_RESOURCE_IN_USE;
    }

    pins[i] = PIN_NUMBER(pin_number);
  }

  RELEASE_LOCK(gpio_bank_state[*bank_number].lock);

  return RTEMS_SUCCESSFUL;
}

static rtems_status_code setup_resistor_and_interrupt_configuration(
  uint32_t pin_number,
  rtems_gpio_pull_mode pull_mode,
  rtems_gpio_interrupt_configuration *interrupt_conf
) {
  gpio_pin_interrupt_state *interrupt_state;
  rtems_status_code sc;
  uint32_t bank;

  sc = rtems_gpio_resistor_mode(pin_number, pull_mode);

  if ( sc != RTEMS_SUCCESSFUL ) {
#if defined(DEBUG)
    printk("rtems_gpio_resistor_mode failed with status code %d\n", sc);
#endif

    return RTEMS_UNSATISFIED;
  }

  if ( interrupt_conf != NULL ) {
    bank = BANK_NUMBER(pin_number);

    ACQUIRE_LOCK(gpio_bank_state[bank].lock);

    sc = rtems_gpio_enable_interrupt(
           pin_number,
           interrupt_conf->active_interrupt,
           interrupt_conf->handler_flag,
           interrupt_conf->threaded_interrupts,
           interrupt_conf->handler,
           interrupt_conf->arg
         );

    if ( sc != RTEMS_SUCCESSFUL ) {
      RELEASE_LOCK(gpio_bank_state[bank].lock);

#if defined(DEBUG)
      printk(
        "rtems_gpio_enable_interrupt failed with status code %d\n",
        sc
      );
#endif

      return RTEMS_UNSATISFIED;
    }

    interrupt_state = gpio_pin_state[pin_number].interrupt_state;

    interrupt_state->debouncing_tick_count =
      interrupt_conf->debounce_clock_tick_interval;

    interrupt_state->last_isr_tick = 0;

    RELEASE_LOCK(gpio_bank_state[bank].lock);
  }

  return RTEMS_SUCCESSFUL;
}

static rtems_status_code gpio_multi_select(
  const rtems_gpio_pin_conf *pins,
  uint8_t pin_count,
  bool on_group
) {
  rtems_status_code sc;
  uint32_t pin_number;
  uint32_t bank;
  uint8_t i;

  /* If the BSP has multi select capabilities. */
#ifdef BSP_GPIO_PINS_PER_SELECT_BANK
  rtems_gpio_multiple_pin_select
    pin_data[GPIO_SELECT_BANK_COUNT][BSP_GPIO_PINS_PER_SELECT_BANK];
  rtems_gpio_specific_data *bsp_data;

  /* Since each platform may have more than two functions to assign to a pin,
   * each pin requires more than one bit in the selection register to
   * properly assign a function to it.
   * Therefore a selection bank (pin selection register) will support fewer pins
   * than a regular bank, meaning that there will be more selection banks than
   * regular banks, which have to be handled separately.
   *
   * This field records the select bank number relative to the GPIO bank. */
  uint32_t select_bank;
  uint32_t bank_number;
  uint32_t select_bank_counter[GPIO_SELECT_BANK_COUNT];
  uint32_t select_count;
  uint32_t pin;

  if ( pin_count == 0 ) {
    return RTEMS_SUCCESSFUL;
  }

  for ( i = 0; i < GPIO_SELECT_BANK_COUNT; ++i ) {
    select_bank_counter[i] = 0;
  }

  for ( i = 0; i < pin_count; ++i ) {
    pin_number = pins[i].pin_number;

    if ( pin_number >= BSP_GPIO_PIN_COUNT ) {
      return RTEMS_INVALID_ID;
    }

    bank = BANK_NUMBER(pin_number);
    pin = PIN_NUMBER(pin_number);

    if ( i == 0 ) {
      bank_number = bank;

      ACQUIRE_LOCK(gpio_bank_state[bank].lock);
    }
    else if ( bank != bank_number ) {
      RELEASE_LOCK(gpio_bank_state[bank_number].lock);

      return RTEMS_UNSATISFIED;
    }

    /* If the pin is already being used returns with an error. */
    if ( gpio_pin_state[pin_number].pin_function != NOT_USED ) {
      RELEASE_LOCK(gpio_bank_state[bank_number].lock);

      return RTEMS_RESOURCE_IN_USE;
    }

    select_bank = (pin_number / BSP_GPIO_PINS_PER_SELECT_BANK) -
                  (bank * GPIO_SELECT_BANK_COUNT);

    select_count = select_bank_counter[select_bank];

    pin_data[select_bank][select_count].pin_number = pin_number;
    pin_data[select_bank][select_count].function = pins[i].function;

    if ( pins[i].function == BSP_SPECIFIC ) {
      bsp_data = (rtems_gpio_specific_data *) pins[i].bsp_specific;

      if ( bsp_data == NULL ) {
        RELEASE_LOCK(gpio_bank_state[bank_number].lock);

        return RTEMS_UNSATISFIED;
      }

      pin_data[select_bank][select_count].io_function = bsp_data->io_function;
      pin_data[select_bank][select_count].bsp_specific = bsp_data->pin_data;
    }
    else {
      /* io_function takes a dummy value, as it will not be used. */
      pin_data[select_bank][select_count].io_function = 0;
      pin_data[select_bank][select_count].bsp_specific = pins[i].bsp_specific;
    }

    ++select_bank_counter[select_bank];
  }

  for ( i = 0; i < GPIO_SELECT_BANK_COUNT; ++i ) {
    if ( select_bank_counter[i] == 0 ) {
      continue;
    }

    sc = rtems_gpio_bsp_multi_select(
           pin_data[i], select_bank_counter[i], i +
           (bank_number * GPIO_SELECT_BANK_COUNT)
         );

    if ( sc != RTEMS_SUCCESSFUL ) {
      RELEASE_LOCK(gpio_bank_state[bank_number].lock);

      return sc;
    }
  }

  for ( i = 0; i < pin_count; ++i ) {
    pin_number = pins[i].pin_number;

    /* Fill other pin state information. */
    gpio_pin_state[pin_number].pin_function = pins[i].function;
    gpio_pin_state[pin_number].logic_invert = pins[i].logic_invert;
    gpio_pin_state[pin_number].on_group = on_group;

    sc = setup_resistor_and_interrupt_configuration(
           pin_number,
           pins[i].pull_mode,
           pins[i].interrupt
         );

    if ( sc != RTEMS_SUCCESSFUL ) {
      RELEASE_LOCK(gpio_bank_state[bank_number].lock);

      return sc;
    }

    bank = BANK_NUMBER(pin_number);
    pin = PIN_NUMBER(pin_number);

    if ( pins[i].function == DIGITAL_OUTPUT ) {
      if ( pins[i].output_enabled == true ) {
        sc = rtems_gpio_bsp_set(bank, pin);
      }
      else {
        sc = rtems_gpio_bsp_clear(bank, pin);
      }

      if ( sc != RTEMS_SUCCESSFUL ) {
        RELEASE_LOCK(gpio_bank_state[bank_number].lock);

        return sc;
      }
    }
  }

  RELEASE_LOCK(gpio_bank_state[bank_number].lock);

  return sc;

  /* If the BSP does not provide pin multi-selection,
   * configures each pin sequentially. */
#else
  for ( i = 0; i < pin_count; ++i ) {
    pin_number = pins[i].pin_number;

    if ( pin_number >= BSP_GPIO_PIN_COUNT ) {
      return RTEMS_INVALID_ID;
    }

    bank = BANK_NUMBER(pin_number);

    ACQUIRE_LOCK(gpio_bank_state[bank].lock);

    /* If the pin is already being used returns with an error. */
    if ( gpio_pin_state[pin_number].pin_function != NOT_USED ) {
      RELEASE_LOCK(gpio_bank_state[bank].lock);

      return RTEMS_RESOURCE_IN_USE;
    }
  }

  for ( i = 0; i < pin_count; ++i ) {
    sc = rtems_gpio_request_configuration(&pins[i]);

    if ( sc != RTEMS_SUCCESSFUL ) {
      return sc;
    }

    gpio_pin_state[pins[i].pin_number].on_group = on_group;
  }

  return RTEMS_SUCCESSFUL;
#endif
}

rtems_status_code rtems_gpio_initialize(void)
{
  rtems_status_code sc;
  uint32_t i;

  if ( _Atomic_Flag_test_and_set(&init_flag, ATOMIC_ORDER_RELAXED) == true ) {
    return RTEMS_SUCCESSFUL;
  }

  for ( i = 0; i < GPIO_BANK_COUNT; ++i ) {
    sc = CREATE_LOCK(
           rtems_build_name('G', 'I', 'N', 'T'),
           &gpio_bank_state[i].lock
         );

    if ( sc != RTEMS_SUCCESSFUL ) {
      return sc;
    }

    gpio_bank_state[i].bank_number = i;
    gpio_bank_state[i].interrupt_counter = 0;

    /* The threaded_interrupts field is initialized during
     * rtems_gpio_enable_interrupt(), as its value is never used before. */
  }

  for ( i = 0; i < BSP_GPIO_PIN_COUNT; ++i ) {
    gpio_pin_state[i].pin_function = NOT_USED;
    gpio_pin_state[i].resistor_mode = NO_PULL_RESISTOR;
    gpio_pin_state[i].logic_invert = false;
    gpio_pin_state[i].on_group = false;
    gpio_pin_state[i].interrupt_state = NULL;
  }

  /* Initialize GPIO groups chain. */
  rtems_chain_initialize_empty(&gpio_group);

  return RTEMS_SUCCESSFUL;
}

rtems_gpio_group *rtems_gpio_create_pin_group(void)
{
  struct rtems_gpio_group *group;

  group = (struct rtems_gpio_group *) malloc(sizeof(struct rtems_gpio_group));

  return group;
}

rtems_status_code rtems_gpio_define_pin_group(
  const rtems_gpio_group_definition *group_definition,
  rtems_gpio_group *group
) {
  rtems_status_code sc;

  if ( group_definition == NULL || group == NULL ) {
    return RTEMS_UNSATISFIED;
  }

  if (
      group_definition->input_count == 0 &&
      group_definition->output_count == 0 &&
      group_definition->bsp_specific_pin_count == 0
  ) {
    return RTEMS_UNSATISFIED;
  }

  group->input_count = group_definition->input_count;

  if ( group->input_count > 0 ) {
    group->digital_inputs =
      (uint32_t *) malloc(group->input_count * sizeof(uint32_t));

    /* Evaluate if the pins that will constitute the group are available and
     * that pins with the same function within the group all belong
     * to the same pin group. */
    sc = check_same_bank_and_availability(
           group_definition->digital_inputs,
           group->input_count,
           &group->digital_input_bank,
           group->digital_inputs
         );

    if ( sc != RTEMS_SUCCESSFUL ) {
      return sc;
    }
  }
  else {
    group->digital_inputs = NULL;
  }

  group->output_count = group_definition->output_count;

  if ( group->output_count > 0 ) {
    group->digital_outputs =
      (uint32_t *) malloc(group->output_count * sizeof(uint32_t));

    sc = check_same_bank_and_availability(
           group_definition->digital_outputs,
           group->output_count,
           &group->digital_output_bank,
           group->digital_outputs
         );

    if ( sc != RTEMS_SUCCESSFUL ) {
      return sc;
    }
  }
  else {
    group->digital_outputs = NULL;
  }

  group->bsp_specific_pin_count = group_definition->bsp_specific_pin_count;

  if ( group->bsp_specific_pin_count > 0 ) {
    group->bsp_speficifc_pins =
      (uint32_t *) malloc(
                     group->bsp_specific_pin_count *
                     sizeof(uint32_t)
                   );

    sc = check_same_bank_and_availability(
           group_definition->bsp_specifics,
           group->bsp_specific_pin_count,
           &group->bsp_specific_bank,
           group->bsp_speficifc_pins
         );

    if ( sc != RTEMS_SUCCESSFUL ) {
      return sc;
    }
  }
  else {
    group->bsp_speficifc_pins = NULL;
  }

  /* Request the pins. */
  sc = gpio_multi_select(
         group_definition->digital_inputs,
         group_definition->input_count,
         true
       );

  if ( sc != RTEMS_SUCCESSFUL ) {
    return RTEMS_UNSATISFIED;
  }

  sc = gpio_multi_select(
         group_definition->digital_outputs,
         group_definition->output_count,
         true
       );

  if ( sc != RTEMS_SUCCESSFUL ) {
    sc = rtems_gpio_release_multiple_pins(
           group_definition->digital_inputs,
           group_definition->input_count
         );

    assert ( sc == RTEMS_SUCCESSFUL );

    return RTEMS_UNSATISFIED;
  }

  sc = gpio_multi_select(
         group_definition->bsp_specifics,
         group_definition->bsp_specific_pin_count,
         true
       );

  if ( sc != RTEMS_SUCCESSFUL ) {
    sc = rtems_gpio_release_multiple_pins(
           group_definition->digital_inputs,
           group_definition->input_count
         );

    assert ( sc == RTEMS_SUCCESSFUL );

    sc = rtems_gpio_release_multiple_pins(
           group_definition->digital_outputs,
           group_definition->output_count
         );

    assert ( sc == RTEMS_SUCCESSFUL );

    return RTEMS_UNSATISFIED;
  }

  /* Create group lock. */
  sc = CREATE_LOCK(rtems_build_name('G', 'R', 'P', 'L'), &group->group_lock);

  if ( sc != RTEMS_SUCCESSFUL ) {
    return sc;
  }

  rtems_chain_append(&gpio_group, &group->node);

  return RTEMS_SUCCESSFUL;
}

rtems_status_code rtems_gpio_write_group(uint32_t data, rtems_gpio_group *group)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;
  uint32_t set_bitmask;
  uint32_t clear_bitmask;
  uint32_t bank;
  uint32_t pin;
  uint8_t i;

  if ( group->output_count == 0 ) {
    return RTEMS_NOT_DEFINED;
  }

  bank = group->digital_output_bank;

  /* Acquire bank lock for the digital output pins. */
  ACQUIRE_LOCK(gpio_bank_state[bank].lock);

  /* Acquire group lock. */
  ACQUIRE_LOCK(group->group_lock);

  set_bitmask = 0;
  clear_bitmask = 0;

  for ( i = 0; i < group->output_count; ++i ) {
    pin = group->digital_outputs[i];

    if ( (data & (1 << i)) == 0 ) {
      clear_bitmask |= (1 << pin);
    }
    else {
      set_bitmask |= (1 << pin);
    }
  }

  /* Set the logical highs. */
  if ( set_bitmask > 0 ) {
    sc = rtems_gpio_bsp_multi_set(bank, set_bitmask);

    if ( sc != RTEMS_SUCCESSFUL ) {
      RELEASE_LOCK(group->group_lock);
      RELEASE_LOCK(gpio_bank_state[bank].lock);

      return sc;
    }
  }

  /* Set the logical lows. */
  if ( clear_bitmask > 0 ) {
    sc = rtems_gpio_bsp_multi_clear(bank, clear_bitmask);

    if ( sc != RTEMS_SUCCESSFUL ) {
      RELEASE_LOCK(group->group_lock);
      RELEASE_LOCK(gpio_bank_state[bank].lock);

      return sc;
    }
  }

  RELEASE_LOCK(group->group_lock);
  RELEASE_LOCK(gpio_bank_state[bank].lock);

  return RTEMS_SUCCESSFUL;
}

uint32_t rtems_gpio_read_group(rtems_gpio_group *group)
{
  uint32_t read_bitmask;
  uint32_t bank;
  uint32_t pin;
  uint32_t rv;
  uint8_t i;

  if ( group->input_count == 0 ) {
    return GPIO_INPUT_ERROR;
  }

  bank = group->digital_input_bank;

  /* Acquire bank lock for the digital input pins. */
  ACQUIRE_LOCK(gpio_bank_state[bank].lock);

  /* Acquire group lock. */
  ACQUIRE_LOCK(group->group_lock);

  read_bitmask = 0;

  for ( i = 0; i < group->input_count; ++i ) {
    pin = group->digital_inputs[i];

    read_bitmask |= (1 << pin);
  }

  rv = rtems_gpio_bsp_multi_read(bank, read_bitmask);

  RELEASE_LOCK(gpio_bank_state[bank].lock);
  RELEASE_LOCK(group->group_lock);

  return rv;
}

rtems_status_code rtems_gpio_group_bsp_specific_operation(
  rtems_gpio_group *group,
  void 
/* ... truncated ... */
```
