/* ds3231_eval_adapter.c — Evaluation adapter for zephyr */
#include "drivergen_eval_adapter.h"
#include "ds3231_ref.h"
#include "zephyr.h"

static struct device _i2c_dev = {.name = "i2c1"};

static struct ds3231_device g_eval_dev;

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

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    int err = ds3231_init(&g_eval_dev, &_i2c_dev, DS3231_ADDR_DEFAULT);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_get_time(drivergen_eval_time_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    struct ds3231_time t;
    if (ds3231_read_time(&g_eval_dev, &t) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    out->year     = (uint16_t)(2000u + (uint16_t)t.year);
    out->month    = t.month;
    out->day      = t.date;     /* DS3231 "date" = day-of-month */
    out->hour     = t.hours;
    out->minute   = t.minutes;
    out->second   = t.seconds;
    out->weekday  = t.day;      /* DS3231 "day"  = day-of-week   */
    out->reserved = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_set_time(const drivergen_eval_time_t *in) {
    (void)in;
    return DRIVERGEN_EVAL_ERR_UNSUPPORTED;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
