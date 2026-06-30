/* lsm303dlhc_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the LSM303DLHC
 * RT-Thread reference driver (I2C multi_channel, 6 channels:
 * accel_{x,y,z} + mag_{x,y,z}).
 *
 * Reference driver API:
 *   rt_err_t lsm303dlhc_init(dev, const char *bus_name);
 *   rt_err_t lsm303dlhc_probe(dev);
 *   rt_err_t lsm303dlhc_accel_start(dev);
 *   rt_err_t lsm303dlhc_accel_read_raw(dev, struct lsm303dlhc_xyz *accel);
 *   rt_err_t lsm303dlhc_mag_start(dev);
 *   rt_err_t lsm303dlhc_mag_read_raw(dev, struct lsm303dlhc_xyz *mag);
 *
 * Channel order (matches oracle meta.channels[]):
 *   0..2 -> accel_{x,y,z}
 *   3..5 -> mag_{x,y,z}
 *
 * Cache semantics: the first read_channel(0) issues BOTH an accel read
 * and a magnetometer read (two separate I2C transactions, since
 * accel lives at addr 0x19 and mag at 0x1E on this part).
 */
#include "drivergen_eval_adapter.h"
#include "lsm303dlhc_ref.h"

#define LSM303_EVAL_CHANNEL_COUNT 6

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
    if (lsm303dlhc_accel_read_raw(&g_eval_dev, &accel) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (lsm303dlhc_mag_read_raw(&g_eval_dev, &mag) != RT_EOK) {
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
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (lsm303dlhc_init(&g_eval_dev, bus_name) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (lsm303dlhc_probe(&g_eval_dev) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (lsm303dlhc_accel_start(&g_eval_dev) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (lsm303dlhc_mag_start(&g_eval_dev) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
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
