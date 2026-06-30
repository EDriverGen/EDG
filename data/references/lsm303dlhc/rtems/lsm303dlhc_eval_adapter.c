#include "drivergen_eval_adapter.h"
#include "lsm303dlhc_ref.h"

#define LSM303_EVAL_CHANNEL_COUNT 6

static struct lsm303dlhc_device g_dev;
static int32_t g_cached[LSM303_EVAL_CHANNEL_COUNT];
static int g_sample_valid;

static const drivergen_eval_channel_t g_channels[LSM303_EVAL_CHANNEL_COUNT] = {
    {"accel_x", "raw", 0},
    {"accel_y", "raw", 0},
    {"accel_z", "raw", 0},
    {"mag_x", "raw", 0},
    {"mag_y", "raw", 0},
    {"mag_z", "raw", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "lsm303dlhc",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = LSM303_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "accel_x",
    .primary_unit       = "raw",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int lsm303_eval_refresh_cache(void)
{
    struct lsm303dlhc_xyz accel = {0, 0, 0};
    struct lsm303dlhc_xyz mag = {0, 0, 0};
    if (lsm303dlhc_accel_read_raw(&g_dev, &accel) != 0 ||
        lsm303dlhc_mag_read_raw(&g_dev, &mag) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = accel.x; g_cached[1] = accel.y; g_cached[2] = accel.z;
    g_cached[3] = mag.x; g_cached[4] = mag.y; g_cached[5] = mag.z;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name)
{
    const char *path = (bus_name != NULL && bus_name[0] != '\0') ? bus_name : "/dev/i2c-0";
    if (lsm303dlhc_init(&g_dev, path) != 0 ||
        lsm303dlhc_probe(&g_dev) != 0 ||
        lsm303dlhc_accel_start(&g_dev) != 0 ||
        lsm303dlhc_mag_start(&g_dev) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out)
{
    if (out == NULL || channel_id < 0 || channel_id >= LSM303_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = lsm303_eval_refresh_cache();
        if (err != DRIVERGEN_EVAL_OK) {
            return err;
        }
    }
    *out = g_cached[channel_id];
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
