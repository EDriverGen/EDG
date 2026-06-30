/* max31855_eval_adapter.c — Evaluation adapter for riot */
#include "drivergen_eval_adapter.h"
#include "max31855_ref.h"
#include "riot.h"

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
    (void)bus_name;
    spi_init(0);
    int err = max31855_init(&g_eval_dev, 0, 0);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int32_t mc = 0;
    int err = max31855_read_thermocouple(&g_eval_dev, &mc);
    if (err != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)mc;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
