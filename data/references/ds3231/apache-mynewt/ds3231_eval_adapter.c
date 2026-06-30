#include "drivergen_eval_adapter.h"
#include "ds3231_ref.h"

static struct ds3231_device g_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "ds3231",
    .eval_class         = DRIVERGEN_EVAL_CLASS_RTC,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = NULL,
    .primary_unit       = NULL,
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    return ds3231_init(&g_dev, 0, DS3231_ADDR_DEFAULT) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_get_time(drivergen_eval_time_t *out)
{
    struct ds3231_time t;
    if (out == 0) return DRIVERGEN_EVAL_ERR_INVALID;
    if (ds3231_read_time(&g_dev, &t) != 0) return DRIVERGEN_EVAL_ERR_IO;
    out->year = (uint16_t)(2000U + t.year);
    out->month = t.month;
    out->day = t.date;
    out->hour = t.hours;
    out->minute = t.minutes;
    out->second = t.seconds;
    out->weekday = t.day;
    out->reserved = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_set_time(const drivergen_eval_time_t *in)
{
    (void)in;
    return DRIVERGEN_EVAL_ERR_UNSUPPORTED;
}

int drivergen_eval_cleanup(void)
{
    return DRIVERGEN_EVAL_OK;
}
