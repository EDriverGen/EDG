/* bh1750_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the BH1750 RT-Thread
 * reference driver. Provides the minimal eval ABI surface for the
 * "single_channel" eval_class.
 *
 * This file is the human-baseline counterpart to what
 * `drivergen/codegen/adapter_generator.py` will auto-emit in v2; it lets
 * the v2 evaluation framework grade reference drivers on the same scale
 * as DriverGen-generated drivers.
 *
 * Reference driver API:
 *   rt_err_t bh1750_init(struct bh1750_device *dev,
 *                        const char *bus_name, rt_uint8_t addr);
 *   rt_err_t bh1750_read_raw(struct bh1750_device *dev, rt_uint16_t *raw);
 */
#include "drivergen_eval_adapter.h"
#include "bh1750_ref.h"

static struct bh1750_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "bh1750",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "ambient_light",
    .primary_unit       = "lux_raw",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    rt_err_t err = bh1750_init(&g_eval_dev, bus_name, BH1750_DEFAULT_ADDR);
    return (err == RT_EOK) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_uint16_t raw_u16 = 0;
    rt_err_t err = bh1750_read_raw(&g_eval_dev, &raw_u16);
    if (err != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)raw_u16;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
