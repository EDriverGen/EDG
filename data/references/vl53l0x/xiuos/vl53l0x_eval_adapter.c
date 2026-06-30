/* vl53l0x_eval_adapter.c — Evaluation adapter for xiuos */
#include "drivergen_eval_adapter.h"
#include "vl53l0x_ref.h"
#include "xiuos.h"

static struct vl53l0x_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "vl53l0x",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "distance",
    .primary_unit       = "mm",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

#define VL53L0X_DEFAULT_ADDR  0x29

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    int err = vl53l0x_init(&g_eval_dev, bus_name, VL53L0X_ADDR_DEFAULT);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    uint16_t mm = 0;
    if (vl53l0x_read_range_mm(&g_eval_dev, &mm) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)mm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
