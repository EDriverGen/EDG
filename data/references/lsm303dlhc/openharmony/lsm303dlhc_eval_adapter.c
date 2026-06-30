/* lsm303dlhc_eval_adapter.c — Evaluation adapter for openharmony */
#include "drivergen_eval_adapter.h"
#include "lsm303dlhc_ref.h"
#include "openharmony_liteosm.h"

#define LSM303_EVAL_CHANNEL_COUNT 6

static DevHandle _i2c_handle;

static struct lsm303dlhc_device g_eval_dev;
static int32_t g_cached[LSM303_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[LSM303_EVAL_CHANNEL_COUNT] = {
    {"accel_x", "raw", 0},
    {"accel_y", "raw", 0},
    {"accel_z", "raw", 0},
    {"mag_x",   "raw", 0},
    {"mag_y",   "raw", 0},
    {"mag_z",   "raw", 0},
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

static int lsm303_eval_refresh_cache(void) {
    struct lsm303dlhc_xyz accel = {0}, mag = {0};
    if (lsm303dlhc_accel_read_raw(&g_eval_dev, &accel) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (lsm303dlhc_mag_read_raw(&g_eval_dev, &mag) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = (int32_t)accel.x;
    g_cached[1] = (int32_t)accel.y;
    g_cached[2] = (int32_t)accel.z;
    g_cached[3] = (int32_t)mag.x;
    g_cached[4] = (int32_t)mag.y;
    g_cached[5] = (int32_t)mag.z;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    _i2c_handle = I2cOpen(0);
    int err = lsm303dlhc_init(&g_eval_dev, _i2c_handle, LSM303DLHC_ADDR_ACCEL);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= LSM303_EVAL_CHANNEL_COUNT) {
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

int drivergen_eval_cleanup(void) {
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
