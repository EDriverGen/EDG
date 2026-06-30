/* Stub mirror of RT-Thread's `components/libc/posix/delay/delay.h`.
 * Minimal surface: usleep / sleep / msleep prototypes (POSIX-style).
 */
#ifndef __RTTHREAD_POSIX_DELAY_STUB_H__
#define __RTTHREAD_POSIX_DELAY_STUB_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int  usleep(unsigned int usec);
unsigned int sleep(unsigned int seconds);
unsigned int msleep(unsigned int msec);

#ifdef __cplusplus
}
#endif

#endif
