/*
 * Minimal RTEMS stub header for DriverGen evaluation.
 */
#ifndef DRIVERGEN_RTEMS_STUB_H
#define DRIVERGEN_RTEMS_STUB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int rtems_status_code;
typedef uint32_t rtems_interval;
typedef uint32_t rtems_id;
typedef uint32_t rtems_name;

typedef struct {
    void *owner;
} rtems_recursive_mutex;

#define RTEMS_SUCCESSFUL 0
#define RTEMS_TIMEOUT 6
#define RTEMS_INVALID_ID 4
#define RTEMS_MILLISECONDS_TO_TICKS(ms) ((rtems_interval)(ms))
#define RTEMS_MICROSECONDS_TO_TICKS(us) ((rtems_interval)(((us) + 999U) / 1000U))

rtems_status_code rtems_task_wake_after(rtems_interval ticks);
rtems_interval rtems_clock_get_ticks_per_second(void);

int printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
