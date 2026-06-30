/* lm75a_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the LM75A RT-Thread
 * reference driver. Provides the minimal eval ABI surface for the
 * "single_channel" eval_class (primary channel: temperature).
 *
 * This is the human-baseline counterpart to what
 * `drivergen/codegen/adapter_generator.py` will auto-emit in v2. It lets
 * the v2 evaluation framework grade reference drivers on the same scale
 * as DriverGen-generated drivers.
 *
 * Reference driver API:
 *   rt_err_t lm75a_init(struct lm75a_device *dev,
 *                       const char *bus_name, rt_uint8_t addr);
 *   rt_err_t lm75a_read_raw(struct lm75a_device *dev, rt_int16_t *raw);
 *
 * `lm75a_read_raw` returns the 11-bit signed temperature in 0.125 C
 * units (e.g. 200 for +25.000 C, -200 for -25.000 C). The adapter
 * sign-extends this int16 into the int32 surface expected by the
 * eval ABI — no unit conversion is performed at this layer.
 */
#include "drivergen_eval_adapter.h"
#include "lm75a_ref.h"

static struct lm75a_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "lm75a",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "temp",
    .primary_unit       = "eighth_celsius",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_err_t err = lm75a_init(&g_eval_dev, bus_name, LM75A_DEFAULT_ADDR);
    return (err == RT_EOK) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_int16_t raw_i16 = 0;
    rt_err_t err = lm75a_read_raw(&g_eval_dev, &raw_i16);
    if (err != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)raw_i16;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
