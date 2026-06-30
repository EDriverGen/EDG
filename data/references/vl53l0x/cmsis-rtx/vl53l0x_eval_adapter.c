#include "drivergen_eval_adapter.h"
#include "vl53l0x_ref.h"

static I2C_HandleTypeDef g_i2c;
static struct vl53l0x_device g_dev;

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

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    return vl53l0x_init(&g_dev, &g_i2c, VL53L0X_ADDR_DEFAULT) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out)
{
    uint16_t mm = 0;
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (vl53l0x_read_range_mm(&g_dev, &mm) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)mm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    return DRIVERGEN_EVAL_OK;
}
