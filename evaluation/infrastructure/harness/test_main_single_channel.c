/* test_main_single_channel.c
 *
 * Universal harness for eval_class = "single_channel" devices.
 *
 * Compile-time configuration (injected by evaluation/runtime/compile.py
 * via -D flags; defaults below allow standalone compilation against I2C):
 *
 *   DRIVERGEN_BUS_HEADER         "hw_i2c.h" / "hw_spi.h" / "hw_uart.h"
 *   DRIVERGEN_BUS_INIT_CALL      hw_i2c1_init() / hw_spi1_init() / ...
 *   DRIVERGEN_BUS_NAME           "i2c1" / "spi1" / "uart1" / "PB5"
 *
 * Output protocol (parsed by evaluation/runtime/renode_exec.py):
 *
 *   DRIVER_TEST START
 *   device=<id>
 *   eval_class=<class>
 *   read_raw=<int>
 *   RESULT: PASS  | RESULT: FAIL
 *   DRIVER_TEST DONE
 *
 * Any line starting with "ERROR:" is treated as a failure marker.
 *
 * This file is intentionally bus-agnostic; the only bus-specific things
 * are the header include and init call, both injected by the runtime.
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

    int err = drivergen_eval_init(DRIVERGEN_BUS_NAME);
    if (err != 0) {
        hw_uart2_puts("ERROR: drivergen_eval_init failed (err=");
        hw_uart2_print_int(err);
        hw_uart2_puts(")\r\n");
        hw_uart2_puts("RESULT: FAIL\r\n");
        goto done;
    }

    int32_t raw = 0;
    err = drivergen_eval_read_raw_i32(&raw);

    hw_uart2_puts("read_raw=");
    hw_uart2_print_int(raw);
    hw_uart2_puts("\r\n");

    hw_uart2_puts("read_err=");
    hw_uart2_print_int(err);
    hw_uart2_puts("\r\n");

    if (err == 0) {
        hw_uart2_puts("RESULT: PASS\r\n");
    } else {
        hw_uart2_puts("RESULT: ERR\r\n");
    }

    drivergen_eval_cleanup();

done:
    hw_uart2_puts("DRIVER_TEST DONE\r\n");
    while (1) { }
    return 0;
}
