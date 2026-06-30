/* mcp3008_eval_adapter.c — Evaluation adapter for chibios */
#include "drivergen_eval_adapter.h"
#include "mcp3008_ref.h"
#include "hal.h"

#define MCP3008_EVAL_CHANNEL_COUNT MCP3008_CHANNELS  /* 8 */
#define MCP3008_EVAL_VREF_MV       3300

static SPIConfig _spi_cfg = {0, 0, 0, 0};

static struct mcp3008_device g_eval_dev;

static const drivergen_eval_channel_t g_channels[MCP3008_EVAL_CHANNEL_COUNT] = {
    {"ch0", "raw", 0},
    {"ch1", "raw", 0},
    {"ch2", "raw", 0},
    {"ch3", "raw", 0},
    {"ch4", "raw", 0},
    {"ch5", "raw", 0},
    {"ch6", "raw", 0},
    {"ch7", "raw", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "mcp3008",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = MCP3008_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "ch0",
    .primary_unit       = "raw",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    spiStart(&SPID1, &_spi_cfg);
    int err = mcp3008_init(&g_eval_dev, &SPID1, &_spi_cfg, 3300);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= MCP3008_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    uint16_t raw = 0;
    if (mcp3008_read_raw(&g_eval_dev,
                         (uint8_t)channel_id,
                         MCP3008_SINGLE,
                         &raw) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)raw;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
