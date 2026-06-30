/*
 * MCP3008 sample for OpenHarmony HDF
 */
#include "mcp3008_ref.h"
#include "hdf_log.h"
#define HDF_LOG_TAG mcp3008_sample

int mcp3008_sample(void)
{
    struct mcp3008_device adc;
    if (mcp3008_init(&adc, 0, 0, 3300) != HDF_SUCCESS)
    { HDF_LOGE("Init failed"); return HDF_FAILURE; }

    for (int i = 0; i < 8; i++) {
        uint16_t mv;
        if (mcp3008_read_voltage(&adc, i, &mv) == HDF_SUCCESS)
            HDF_LOGI("CH%d: %d.%03d V", i, mv/1000, mv%1000);
    }
    mcp3008_deinit(&adc);
    return HDF_SUCCESS;
}
