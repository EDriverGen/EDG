/* max31855_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the MAX31855
 * RT-Thread reference driver (SPI stream-mode, single_channel).
 *
 * Reference driver API:
 *   rt_err_t max31855_init(dev, const char *device_name);
 *   rt_err_t max31855_read_thermocouple(dev, rt_int32_t *temp_mc);
 *
 * The MAX31855 is a read-only SPI thermocouple-to-digital chip: every
 * CS-low → 4-byte SPI frame → CS-high sequence yields one temperature
 * reading. `bus_name` from the harness is passed straight through as
 * `device_name` to the driver's init.
 *
 * Units: the driver returns mC directly (matches oracle's `mC` spec).
 */
#include "drivergen_eval_adapter.h"
#include "max31855_ref.h"

static struct max31855_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "max31855",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "temp_thermocouple",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (max31855_init(&g_eval_dev, bus_name) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_int32_t mc = 0;
    rt_err_t err = max31855_read_thermocouple(&g_eval_dev, &mc);
    if (err != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)mc;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
