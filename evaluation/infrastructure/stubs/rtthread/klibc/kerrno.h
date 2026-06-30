#ifndef DRIVERGEN_STUB_RTTHREAD_KLIBC_KERRNO_H
#define DRIVERGEN_STUB_RTTHREAD_KLIBC_KERRNO_H

/*
 * Upstream RT-Thread moved the RT_E* error-code table into
 * include/klibc/kerrno.h (see commit by Meco Man, 2024-09-22).  Drivers
 * that target recent RT-Thread trees may `#include <klibc/kerrno.h>`
 * directly instead of pulling in the whole of rtthread.h.
 *
 * This stub simply forwards to our canonical rtthread.h, which carries
 * the full RT_E* constant set for cross-compilation.
 */
#include "rtthread.h"

rt_err_t rt_get_errno(void);
void rt_set_errno(rt_err_t no);
int *_rt_errno(void);
const char *rt_strerror(rt_err_t error);

#endif /* DRIVERGEN_STUB_RTTHREAD_KLIBC_KERRNO_H */
