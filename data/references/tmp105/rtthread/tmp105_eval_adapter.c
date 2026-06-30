/* tmp105_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the TMP105 RT-Thread
 * reference driver (I2C single_channel).
 *
 * Reference driver API:
 *   rt_err_t tmp105_init(struct tmp105_device *dev,
 *                        const char *bus_name, rt_uint8_t addr);
 *   rt_err_t tmp105_read_temperature(struct tmp105_device *dev,
 *                                    rt_int32_t *temp_mcelsius);
 *
 * The driver returns temperature directly in millidegree Celsius (mC),
 * matching the oracle's `primary.physical_unit = "mC"`, so no scaling
 * is needed — we just widen rt_int32_t to int32_t (same width).
 */
#include "drivergen_eval_adapter.h"
#include "tmp105_ref.h"

static struct tmp105_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "tmp105",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "temp",
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
    rt_err_t err = tmp105_init(&g_eval_dev, bus_name, TMP105_DEFAULT_ADDR);
    return (err == RT_EOK) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_int32_t mc = 0;
    rt_err_t err = tmp105_read_temperature(&g_eval_dev, &mc);
    if (err != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)mc;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
