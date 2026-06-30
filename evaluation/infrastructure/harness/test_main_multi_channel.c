/* test_main_multi_channel.c
 *
 * Universal harness for eval_class = "multi_channel" devices (e.g. MPU6050).
 *
 * Compile-time configuration matches test_main_single_channel.c
 * (DRIVERGEN_BUS_HEADER / DRIVERGEN_BUS_INIT_CALL / DRIVERGEN_BUS_NAME).
 *
 * Output protocol (USART2, parsed by evaluation/runtime/renode_exec.py):
 *
 *   DRIVER_TEST START
 *   device=<id>
 *   eval_class=multi_channel
 *   channel_count=<N>
 *   read_ch_<channel_id>=<int>    (one line per channel, id from meta.channels[i].id)
 *   read_raw=<int>                (same as channel 0 — backward compat for tools)
 *   read_err=<int>              (nonzero when a channel read returns an error)
 *   RESULT: PASS | RESULT: FAIL | RESULT: ERR
 *   DRIVER_TEST DONE
 *
 * Channel ids are alphanumeric + underscore only in practice; they are
 * emitted verbatim after the "read_ch_" prefix.
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

#include DRIVERGEN_BUS_HEADER

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

    {
        int n = drivergen_eval_meta.channel_count;
        hw_uart2_puts("channel_count=");
        hw_uart2_print_int((int32_t)n);
        hw_uart2_puts("\r\n");

        if (n <= 0 || drivergen_eval_meta.channels == 0) {
            hw_uart2_puts("ERROR: invalid multi_channel metadata\r\n");
            hw_uart2_puts("RESULT: FAIL\r\n");
            goto done;
        }

        int err = drivergen_eval_init(DRIVERGEN_BUS_NAME);
        if (err != 0) {
            hw_uart2_puts("ERROR: drivergen_eval_init failed (err=");
            hw_uart2_print_int(err);
            hw_uart2_puts(")\r\n");
            hw_uart2_puts("RESULT: FAIL\r\n");
            goto done;
        }

        int32_t first_raw = 0;
        int any_fail = 0;

        for (int i = 0; i < n; i++) {
            int32_t raw = 0;
            err = drivergen_eval_read_channel(i, &raw);
            if (err != 0) {
                hw_uart2_puts("read_err=");
                hw_uart2_print_int(err);
                hw_uart2_puts("\r\n");
                hw_uart2_puts("ERROR: drivergen_eval_read_channel returned error (i=");
                hw_uart2_print_int((int32_t)i);
                hw_uart2_puts(" err=");
                hw_uart2_print_int(err);
                hw_uart2_puts(")\r\n");
                any_fail = 1;
                break;
            }
            if (i == 0) {
                first_raw = raw;
            }

            hw_uart2_puts("read_ch_");
            hw_uart2_puts(drivergen_eval_meta.channels[i].id);
            hw_uart2_puts("=");
            hw_uart2_print_int(raw);
            hw_uart2_puts("\r\n");
        }

        if (any_fail) {
            hw_uart2_puts("RESULT: ERR\r\n");
            drivergen_eval_cleanup();
            goto done;
        }

        hw_uart2_puts("read_raw=");
        hw_uart2_print_int(first_raw);
        hw_uart2_puts("\r\n");

        hw_uart2_puts("RESULT: PASS\r\n");
        drivergen_eval_cleanup();
    }

done:
    hw_uart2_puts("DRIVER_TEST DONE\r\n");
    while (1) { }
    return 0;
}
