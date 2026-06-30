#ifndef RT_DBG_H__
#define RT_DBG_H__
/*
 * RT-Thread debug logging stub for cross-compilation testing.
 * Provides LOG_D/LOG_I/LOG_W/LOG_E macros as no-ops.
 *
 * Note on dual-include with `rtthread.h`:
 * - Real RT-Thread: `include/rtthread.h` does NOT define LOG_*; only
 *   `rtdbg.h` defines them.  No conflict.
 * - This stub: `rtthread/rtthread.h` provides `#ifndef`-guarded fallback
 *   `LOG_*` definitions so drivers that only include `rtthread.h` still
 *   compile.  When both headers end up in the same TU, that fallback
 *   would otherwise collide with this header.  We deliberately
 *   `#undef LOG_*` here so the dbg-style definitions always win,
 *   matching the intent of real `rtdbg.h` (it is the canonical home for
 *   `LOG_*`) and avoiding `-Wmacro-redefined` noise that is purely
 *   stub-introduced.  This is a stub-only override that does not change
 *   real RT-Thread behavior in any way.
 */

#undef LOG_D
#undef LOG_I
#undef LOG_W
#undef LOG_E
#undef LOG_RAW
#undef LOG_HEX

/* DEBUG level constants */
#define DBG_ERROR           0
#define DBG_WARNING         1
#define DBG_INFO            2
#define DBG_LOG             3

/* Default level if not set */
#ifndef DBG_LEVEL
#ifdef DBG_LVL
#define DBG_LEVEL           DBG_LVL
#else
#define DBG_LEVEL           DBG_WARNING
#endif
#endif

/* All log macros expand to nothing for stub compilation */
#define LOG_D(fmt, ...)
#define LOG_I(fmt, ...)
#define LOG_W(fmt, ...)
#define LOG_E(fmt, ...)
#define LOG_RAW(...)
#define LOG_HEX(name, width, buf, size)

#define dbg_log_line(lvl, color_n, fmt, ...)
#define dbg_raw(...)

#endif /* RT_DBG_H__ */
