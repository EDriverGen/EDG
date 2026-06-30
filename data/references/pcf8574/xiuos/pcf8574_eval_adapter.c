/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * pcf8574_eval_adapter.c — Evaluation adapter for xiuos
 */
#include "drivergen_eval_adapter.h"
#include "pcf8574_ref.h"

static struct pcf8574_device g_eval_dev;
static uint8_t g_cache;
static int g_cache_valid = 0;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id     = "pcf8574",
    .eval_class    = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count = 8,
    .channels = (const drivergen_eval_channel_t[]){
        {"p0","bool",0},{"p1","bool",0},{"p2","bool",0},{"p3","bool",0},
        {"p4","bool",0},{"p5","bool",0},{"p6","bool",0},{"p7","bool",0},
    },
    .primary_id    = "p0",
    .primary_unit  = "bool",
    .abi_version_major = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int _refresh(void) {
    int ret = pcf8574_read_port(&g_eval_dev, &g_cache);
    if (ret != 0) return DRIVERGEN_EVAL_ERR_IO;
    g_cache_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    const char *path = (bus_name != NULL && bus_name[0] != '\0')
                       ? bus_name : "/dev/i2c-0";
    int err = pcf8574_init(&g_eval_dev, path, PCF8574_I2C_ADDR);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int idx, int32_t *val) {
    if (idx < 0 || idx > 7 || !val) return DRIVERGEN_EVAL_ERR_INVALID;
    if (!g_cache_valid) { int r = _refresh(); if (r) return r; }
    /* P0=bit0 (LSB) ... P7=bit7 (MSB), per datasheet I/O data bus: P7..P0 */
    *val = (int32_t)((g_cache >> idx) & 1);
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    pcf8574_deinit(&g_eval_dev);
    return DRIVERGEN_EVAL_OK;
}
