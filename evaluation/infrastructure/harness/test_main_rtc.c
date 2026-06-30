/* test_main_rtc.c
 *
 * Universal harness for eval_class = "rtc" devices (e.g. DS3231, DS1307,
 * PCF8563, MCP7940, RX8025).
 *
 * Primary check: drivergen_eval_get_time returns 0 and the fields match
 * what the slave preloaded via mock_preload.
 *
 * Optional round-trip: when DRIVERGEN_RTC_DO_SET is defined, the harness
 * first calls drivergen_eval_set_time with a compile-time constant, then
 * drivergen_eval_get_time to confirm the write path.
 *
 * Compile-time configuration matches other harnesses plus:
 *   DRIVERGEN_RTC_DO_SET           when defined, exercise set_time first
 *   DRIVERGEN_RTC_SET_YEAR         default 2024
 *   DRIVERGEN_RTC_SET_MONTH        default 1
 *   DRIVERGEN_RTC_SET_DAY          default 1
 *   DRIVERGEN_RTC_SET_HOUR         default 0
 *   DRIVERGEN_RTC_SET_MINUTE       default 0
 *   DRIVERGEN_RTC_SET_SECOND       default 0
 *   DRIVERGEN_RTC_SET_WEEKDAY      default 0
 *
 * Output protocol (USART2, parsed by evaluation/runtime/renode_exec.py):
 *
 *   DRIVER_TEST START
 *   device=<id>
 *   eval_class=rtc
 *   [rtc_set_err=<int>]   (only when DRIVERGEN_RTC_DO_SET)
 *   rtc_get_err=<int>
 *   rtc_year=<int>
 *   rtc_month=<int>
 *   rtc_day=<int>
 *   rtc_hour=<int>
 *   rtc_minute=<int>
 *   rtc_second=<int>
 *   rtc_weekday=<int>
 *   RESULT: PASS | RESULT: FAIL
 *   DRIVER_TEST DONE
 */

#include <stdint.h>
#include "drivergen_eval_adapter.h"
#include "hw_uart.h"

#ifndef DRIVERGEN_BUS_HEADER
#define DRIVERGEN_BUS_HEADER "hw_i2c.h"
#endif
#ifndef DRIVERGEN_BUS_INIT_CALL
#define DRIVERGEN_BUS_INIT_CALL hw_i2c1_init()
#endif
#ifndef DRIVERGEN_BUS_NAME
#define DRIVERGEN_BUS_NAME "i2c1"
#endif
#ifndef DRIVERGEN_RTC_SET_YEAR
#define DRIVERGEN_RTC_SET_YEAR 2024u
#endif
#ifndef DRIVERGEN_RTC_SET_MONTH
#define DRIVERGEN_RTC_SET_MONTH 1u
#endif
#ifndef DRIVERGEN_RTC_SET_DAY
#define DRIVERGEN_RTC_SET_DAY 1u
#endif
#ifndef DRIVERGEN_RTC_SET_HOUR
#define DRIVERGEN_RTC_SET_HOUR 0u
#endif
#ifndef DRIVERGEN_RTC_SET_MINUTE
#define DRIVERGEN_RTC_SET_MINUTE 0u
#endif
#ifndef DRIVERGEN_RTC_SET_SECOND
#define DRIVERGEN_RTC_SET_SECOND 0u
#endif
#ifndef DRIVERGEN_RTC_SET_WEEKDAY
#define DRIVERGEN_RTC_SET_WEEKDAY 0u
#endif

#include DRIVERGEN_BUS_HEADER

static void emit_named_int(const char *name, int32_t v) {
    hw_uart2_puts(name);
    hw_uart2_puts("=");
    hw_uart2_print_int(v);
    hw_uart2_puts("\r\n");
}

int main(void) {
    DRIVERGEN_BUS_INIT_CALL;
    hw_uart2_init();

    hw_uart2_puts("DRIVER_TEST START\r\n");

    hw_uart2_puts("device=");
    hw_uart2_puts(drivergen_eval_meta.device_id);
    hw_uart2_puts("\r\n");

    hw_uart2_puts("eval_class=");
    hw_uart2_puts(drivergen_eval_meta.eval_class);
    hw_uart2_puts("\r\n");

    int err = drivergen_eval_init(DRIVERGEN_BUS_NAME);
    if (err != 0) {
        hw_uart2_puts("ERROR: drivergen_eval_init failed (err=");
        hw_uart2_print_int(err);
        hw_uart2_puts(")\r\n");
        hw_uart2_puts("RESULT: FAIL\r\n");
        goto done;
    }

    int overall_ok = 1;

#ifdef DRIVERGEN_RTC_DO_SET
    drivergen_eval_time_t set_in = {
        .year     = (uint16_t)(DRIVERGEN_RTC_SET_YEAR),
        .month    = (uint8_t )(DRIVERGEN_RTC_SET_MONTH),
        .day      = (uint8_t )(DRIVERGEN_RTC_SET_DAY),
        .hour     = (uint8_t )(DRIVERGEN_RTC_SET_HOUR),
        .minute   = (uint8_t )(DRIVERGEN_RTC_SET_MINUTE),
        .second   = (uint8_t )(DRIVERGEN_RTC_SET_SECOND),
        .weekday  = (uint8_t )(DRIVERGEN_RTC_SET_WEEKDAY),
        .reserved = 0,
    };
    int set_err = drivergen_eval_set_time(&set_in);
    emit_named_int("rtc_set_err", set_err);
    if (set_err != 0) {
        overall_ok = 0;
    }
#endif

    drivergen_eval_time_t t = {0};
    int get_err = drivergen_eval_get_time(&t);
    emit_named_int("rtc_get_err", get_err);
    if (get_err != 0) {
        overall_ok = 0;
    }

    emit_named_int("rtc_year",    (int32_t)t.year);
    emit_named_int("rtc_month",   (int32_t)t.month);
    emit_named_int("rtc_day",     (int32_t)t.day);
    emit_named_int("rtc_hour",    (int32_t)t.hour);
    emit_named_int("rtc_minute",  (int32_t)t.minute);
    emit_named_int("rtc_second",  (int32_t)t.second);
    emit_named_int("rtc_weekday", (int32_t)t.weekday);

    if (overall_ok) {
        hw_uart2_puts("RESULT: PASS\r\n");
    } else {
        hw_uart2_puts("ERROR: rtc checks failed\r\n");
        hw_uart2_puts("RESULT: FAIL\r\n");
    }

    drivergen_eval_cleanup();

done:
    hw_uart2_puts("DRIVER_TEST DONE\r\n");
    while (1) { }
    return 0;
}
