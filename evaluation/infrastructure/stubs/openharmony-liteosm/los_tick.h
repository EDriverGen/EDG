/* OpenHarmony LiteOS-M `los_tick.h` sandbox stub for DriverGen.
 *
 * Real upstream:
 *   data/rtos/openharmony-liteosm-project/kernel_liteos_m/kernel/include/los_tick.h
 *
 * Why this stub exists:
 *   Some generated OpenHarmony drivers include this header to call
 *   `LOS_MDelay(ms)` (and similar tick helpers).  The full upstream header
 *   transitively pulls in `los_error.h` / `los_timer.h` / config-dependent
 *   typedefs that are not part of the sandbox surface.  We therefore mirror
 *   only the user-facing API + the LiteOS scalar typedefs they need; the
 *   most generated driver code exercises `LOS_MDelay` only,
 *   and we expose the rest of the public API for forward compatibility.
 *
 *   1. Real upstream file located in `data/rtos/openharmony-liteosm-project/kernel_liteos_m/kernel/include/los_tick.h` (Apache-2.0)  -- ✓
 *   2. User-facing API reproduced verbatim; kernel-internal symbols (g_sysClock,
 *      OsTickHandler, OsTickTimerInit, ArchTickTimer, etc.) are deliberately
 *      omitted.
 *   3. Symbols added: LOS_MDelay, LOS_UDelay, LOS_TickCountGet,
 *      LOS_CyclePerTickGet, LOS_Tick2MS, LOS_MS2Tick, LOS_CurrNanosec,
 *      LOS_SysCycleGet (all extern declarations, weak fallbacks live in
 *      task_package_helpers.c).
 */
#ifndef _LOS_TICK_STUB_H
#define _LOS_TICK_STUB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* LiteOS scalar typedefs (mirror of los_compiler.h user-facing subset).
 * Guarded so that downstream stubs can re-include without redefinition.
 */
#ifndef LOS_TYPEDEF_GUARD
#define LOS_TYPEDEF_GUARD
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned long long  UINT64;
typedef signed char         INT8;
typedef short               INT16;
typedef int                 INT32;
typedef long long           INT64;
typedef unsigned long       UINTPTR;
typedef unsigned int        BOOL;
#endif

#ifndef VOID
#define VOID void
#endif
#ifndef STATIC_INLINE
#define STATIC_INLINE static inline
#endif

/* User-facing constants from upstream los_tick.h. */
#ifndef OS_SYS_MS_PER_SECOND
#define OS_SYS_MS_PER_SECOND   1000
#endif
#ifndef OS_SYS_US_PER_SECOND
#define OS_SYS_US_PER_SECOND   1000000
#endif
#ifndef OS_SYS_NS_PER_SECOND
#define OS_SYS_NS_PER_SECOND   1000000000
#endif

/* Tick / cycle / delay public API (real declarations from upstream). */
extern UINT64 LOS_SysCycleGet(VOID);
extern UINT64 LOS_TickCountGet(VOID);
extern UINT32 LOS_CyclePerTickGet(VOID);
extern UINT32 LOS_Tick2MS(UINT32 ticks);
extern UINT32 LOS_MS2Tick(UINT32 millisec);
extern VOID   LOS_UDelay(UINT64 microseconds);
extern VOID   LOS_MDelay(UINT32 millisec);
extern UINT64 LOS_CurrNanosec(VOID);

#ifdef __cplusplus
}
#endif

#endif /* _LOS_TICK_STUB_H */
