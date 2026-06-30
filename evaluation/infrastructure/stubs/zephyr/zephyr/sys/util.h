/* Zephyr stub: zephyr/sys/util.h
 *
 * Real source: include/zephyr/sys/util.h in zephyr. The real header pulls
 * in util_macro.h, toolchain.h, __assert.h, types.h, which each pull
 * more Zephyr internals that we don't want to stub out.
 *
 * This minimal version only declares the macros that Zephyr driver code
 * most commonly uses inline — BIT(n), GENMASK, ARRAY_SIZE, MIN/MAX,
 * CLAMP, DIV_ROUND_UP, POINTER_TO_UINT. Extending this file with more
 * macros is OK but prefer *not* to import Zephyr-internal types unless
 * a concrete missing symbol forces it (keeps stub surface small so
 * link-level errors stay meaningful).
 */
#ifndef ZEPHYR_INCLUDE_SYS_UTIL_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BITS_PER_BYTE (__CHAR_BIT__)
#define BITS_PER_LONG (__CHAR_BIT__ * __SIZEOF_LONG__)

#ifndef BIT
#define BIT(n)            (1UL << (n))
#endif
#ifndef BIT_MASK
#define BIT_MASK(n)       (BIT(n) - 1UL)
#endif
#ifndef GENMASK
#define GENMASK(h, l) \
    (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)     (sizeof(a) / sizeof((a)[0]))
#endif
#ifndef MIN
#define MIN(a, b)         (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)         (((a) > (b)) ? (a) : (b))
#endif
#ifndef CLAMP
#define CLAMP(x, lo, hi)  (MIN(MAX((x), (lo)), (hi)))
#endif
#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d)   (((n) + (d) - 1) / (d))
#endif

#define POINTER_TO_UINT(x)  ((uintptr_t)(x))
#define UINT_TO_POINTER(x)  ((void *)(uintptr_t)(x))
#define POINTER_TO_INT(x)   ((intptr_t)(x))
#define INT_TO_POINTER(x)   ((void *)(intptr_t)(x))

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_H_ */
