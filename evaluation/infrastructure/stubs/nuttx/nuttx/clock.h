/* NuttX nuttx/clock.h stub */
#ifndef __NUTTX_CLOCK_STUB_H
#define __NUTTX_CLOCK_STUB_H

#include <stdint.h>

/* NuttX system clock frequency */
#define USEC_PER_TICK   10000
#define TICK_PER_SEC    100
#define MSEC_PER_TICK   10

/* NuttX clock types */
typedef uint32_t clock_t;
typedef uint32_t systime_t;

/* Timer functions */
uint32_t clock_systime_ticks(void);
uint32_t clock_systime_frequency(void);

/* POSIX-like time conversion */
#define USEC2TICK(usec) ((usec) / USEC_PER_TICK)
#define MSEC2TICK(msec) ((msec) / MSEC_PER_TICK)
#define TICK2USEC(tick) ((tick) * USEC_PER_TICK)
#define TICK2MSEC(tick) ((tick) * MSEC_PER_TICK)

#endif
