/* test_main_memory.c
 *
 * Universal harness for eval_class = "memory" devices (e.g. AT24Cxx,
 * FM24CLxx EEPROMs and FRAMs).
 *
 * Compile-time configuration matches test_main_single_channel.c plus
 * two memory-specific knobs:
 *
 *   DRIVERGEN_BUS_HEADER       "hw_i2c.h" / "hw_spi.h" / ...
 *   DRIVERGEN_BUS_INIT_CALL    hw_i2c1_init() / hw_spi1_init() / ...
 *   DRIVERGEN_BUS_NAME         "i2c1" / "spi1" / ...
 *   DRIVERGEN_MEM_PROBE_ADDR   probe read address (default 0x0000)
 *   DRIVERGEN_MEM_PROBE_LEN    probe read length  (default 16, max 256)
 *
 * The probe address / length are static at compile time; per-stimulus
 * variation is expressed through the slave's mock_preload (bytes at
 * the probe region are set by the oracle stimulus, not by this harness).
 *
 * Output protocol (USART2, parsed by evaluation/runtime/renode_exec.py):
 *
 *   DRIVER_TEST START
 *   device=<id>
 *   eval_class=memory
 *   memory_size_bytes=<uint32>
 *   memory_page_bytes=<uint16>
 *   mem_probe_addr=<uint32>
 *   mem_probe_len=<uint16>
 *   mem_read=<hex bytes space-separated, lowercase, no 0x prefix>
 *   RESULT: PASS | RESULT: FAIL
 *   DRIVER_TEST DONE
 *
 * Byte format example (16 bytes starting at 0x0000 preloaded with
 * deadbeef + zeros):
 *
 *   mem_read=de ad be ef 00 00 00 00 00 00 00 00 00 00 00 00
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
#ifndef DRIVERGEN_MEM_PROBE_ADDR
#define DRIVERGEN_MEM_PROBE_ADDR 0x0000u
#endif
#ifndef DRIVERGEN_MEM_PROBE_LEN
#define DRIVERGEN_MEM_PROBE_LEN 16u
#endif

#define DRIVERGEN_MEM_PROBE_MAX 256u

#include DRIVERGEN_BUS_HEADER

static void print_hex8_raw(uint8_t v) {
    static const char hex[] = "0123456789abcdef";
    hw_uart2_putc(hex[(v >> 4) & 0xF]);
    hw_uart2_putc(hex[v & 0xF]);
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

    hw_uart2_puts("memory_size_bytes=");
    hw_uart2_print_int((int32_t)drivergen_eval_meta.memory_size_bytes);
    hw_uart2_puts("\r\n");

    hw_uart2_puts("memory_page_bytes=");
    hw_uart2_print_int((int32_t)drivergen_eval_meta.memory_page_bytes);
    hw_uart2_puts("\r\n");

    uint32_t probe_addr = (uint32_t)(DRIVERGEN_MEM_PROBE_ADDR);
    uint16_t probe_len  = (uint16_t)(DRIVERGEN_MEM_PROBE_LEN);
    if (probe_len > DRIVERGEN_MEM_PROBE_MAX) {
        probe_len = (uint16_t)DRIVERGEN_MEM_PROBE_MAX;
    }

    hw_uart2_puts("mem_probe_addr=");
    hw_uart2_print_int((int32_t)probe_addr);
    hw_uart2_puts("\r\n");
    hw_uart2_puts("mem_probe_len=");
    hw_uart2_print_int((int32_t)probe_len);
    hw_uart2_puts("\r\n");

    int err = drivergen_eval_init(DRIVERGEN_BUS_NAME);
    if (err != 0) {
        hw_uart2_puts("ERROR: drivergen_eval_init failed (err=");
        hw_uart2_print_int(err);
        hw_uart2_puts(")\r\n");
        hw_uart2_puts("RESULT: FAIL\r\n");
        goto done;
    }

    uint8_t buf[DRIVERGEN_MEM_PROBE_MAX];
    for (uint16_t i = 0; i < probe_len; i++) {
        buf[i] = 0;
    }

    err = drivergen_eval_mem_read(probe_addr, buf, probe_len);
    if (err != 0) {
        hw_uart2_puts("ERROR: drivergen_eval_mem_read failed (err=");
        hw_uart2_print_int(err);
        hw_uart2_puts(")\r\n");
        hw_uart2_puts("RESULT: FAIL\r\n");
        drivergen_eval_cleanup();
        goto done;
    }

    hw_uart2_puts("mem_read=");
    for (uint16_t i = 0; i < probe_len; i++) {
        if (i > 0) hw_uart2_putc(' ');
        print_hex8_raw(buf[i]);
    }
    hw_uart2_puts("\r\n");

    hw_uart2_puts("RESULT: PASS\r\n");
    drivergen_eval_cleanup();

done:
    hw_uart2_puts("DRIVER_TEST DONE\r\n");
    while (1) { }
    return 0;
}
