/* mhz19b_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the MH-Z19B
 * RT-Thread reference driver (UART bus, 9-byte fixed framing).
 *
 * Provides the minimal eval ABI surface for the "single_channel"
 * eval_class with primary_id = "co2".
 *
 * Reference driver API:
 *   rt_err_t mhz19b_init(dev, const char *uart_name);
 *   rt_err_t mhz19b_read_co2(dev, rt_uint16_t *ppm);
 *
 * `bus_name` (e.g. "uart1") is passed through unchanged; the reference
 * driver internally resolves it to an `rt_device_t` via
 * `rt_device_find`, configures the port at 9600 8N1, and opens it for
 * interrupt-driven receive.
 *
 * Note on stimuli range: the reference driver's read path verifies both
 * the start-byte framing (0xFF 0x86) and the one-byte checksum, so any
 * oracle stimulus that injects a corrupted frame will cause this
 * adapter to surface DRIVERGEN_EVAL_ERR_IO. That is the intended
 * baseline behaviour — L5 robustness judges test frame corruption
 * independently.
 */
#include "drivergen_eval_adapter.h"
#include "mhz19b_ref.h"

static struct mhz19b_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "mhz19b",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "co2",
    .primary_unit       = "ppm",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_err_t err = mhz19b_init(&g_eval_dev, bus_name);
    return (err == RT_EOK) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_uint16_t ppm = 0;
    rt_err_t err = mhz19b_read_co2(&g_eval_dev, &ppm);
    if (err != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)ppm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
