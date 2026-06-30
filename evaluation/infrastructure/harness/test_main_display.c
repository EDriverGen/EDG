/* test_main_display.c
 *
 * Universal harness for eval_class = "display" devices
 * (OLED/LCD/segment drivers such as SSD1306, SH1106, ST7735, ILI9341,
 *  PCF8574+HD44780, TM1637).
 *
 * Display testing is tricky: the device has no "reading" we can check.
 * We instead verify:
 *   1. drivergen_eval_init  returns 0
 *   2. drivergen_eval_output_frame(payload, len) returns 0
 *   3. (optional) drivergen_eval_read_status returns either 0 with a
 *      status byte, or DRIVERGEN_EVAL_ERR_UNSUPPORTED (-6) — both are
 *      acceptable "operation completed" markers.
 *
 * Deep correctness (did the right bytes reach the right registers) is
 * validated by the L3 protocol judge against the bus trace, NOT here.
 *
 * Compile-time configuration matches other harnesses plus:
 *   DRIVERGEN_DISPLAY_FRAME_LEN   payload length  (default 16, max 256)
 *   DRIVERGEN_DISPLAY_FRAME_FILL  byte fill value (default 0xA5)
 *   DRIVERGEN_DISPLAY_CHECK_STATUS  when defined, call read_status and
 *                                   emit status lines
 *
 * Output protocol (USART2, parsed by evaluation/runtime/renode_exec.py):
 *
 *   DRIVER_TEST START
 *   device=<id>
 *   eval_class=display
 *   frame_len=<int>
 *   output_frame_err=<int>
 *   status_err=<int>     (only when DRIVERGEN_DISPLAY_CHECK_STATUS set)
 *   status=<int>         (only when status read returned 0)
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
#ifndef DRIVERGEN_DISPLAY_FRAME_LEN
#define DRIVERGEN_DISPLAY_FRAME_LEN 16u
#endif
#ifndef DRIVERGEN_DISPLAY_FRAME_FILL
#define DRIVERGEN_DISPLAY_FRAME_FILL 0xA5u
#endif

#define DRIVERGEN_DISPLAY_FRAME_MAX 256u

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

    uint16_t flen = (uint16_t)(DRIVERGEN_DISPLAY_FRAME_LEN);
    if (flen > DRIVERGEN_DISPLAY_FRAME_MAX) {
        flen = (uint16_t)DRIVERGEN_DISPLAY_FRAME_MAX;
    }
    hw_uart2_puts("frame_len=");
    hw_uart2_print_int((int32_t)flen);
    hw_uart2_puts("\r\n");

    int err = drivergen_eval_init(DRIVERGEN_BUS_NAME);
    if (err != 0) {
        hw_uart2_puts("ERROR: drivergen_eval_init failed (err=");
        hw_uart2_print_int(err);
        hw_uart2_puts(")\r\n");
        hw_uart2_puts("RESULT: FAIL\r\n");
        goto done;
    }

    static uint8_t frame[DRIVERGEN_DISPLAY_FRAME_MAX];
    for (uint16_t i = 0; i < flen; i++) {
        frame[i] = (uint8_t)(DRIVERGEN_DISPLAY_FRAME_FILL);
    }

    int frame_err = drivergen_eval_output_frame(frame, flen);
    hw_uart2_puts("output_frame_err=");
    hw_uart2_print_int(frame_err);
    hw_uart2_puts("\r\n");

    int overall_ok = (frame_err == 0);

#ifdef DRIVERGEN_DISPLAY_CHECK_STATUS
    uint8_t status_byte = 0;
    int status_err = drivergen_eval_read_status(&status_byte);
    hw_uart2_puts("status_err=");
    hw_uart2_print_int(status_err);
    hw_uart2_puts("\r\n");
    /* DRIVERGEN_EVAL_ERR_UNSUPPORTED (-6) means "no status on this device",
     * which is legal; only explicit failures (other non-zero) are fatal. */
    if (status_err == 0) {
        hw_uart2_puts("status=");
        hw_uart2_print_int((int32_t)status_byte);
        hw_uart2_puts("\r\n");
    } else if (status_err != DRIVERGEN_EVAL_ERR_UNSUPPORTED) {
        overall_ok = 0;
    }
#endif

    if (overall_ok) {
        hw_uart2_puts("RESULT: PASS\r\n");
    } else {
        hw_uart2_puts("ERROR: display checks failed\r\n");
        hw_uart2_puts("RESULT: FAIL\r\n");
    }

    drivergen_eval_cleanup();

done:
    hw_uart2_puts("DRIVER_TEST DONE\r\n");
    while (1) { }
    return 0;
}
