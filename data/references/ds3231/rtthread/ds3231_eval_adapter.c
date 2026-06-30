/* ds3231_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the DS3231 RT-Thread
 * reference driver. Provides the minimal eval ABI surface for the
 * "rtc" eval_class.
 *
 * Reference driver API:
 *   int ds3231_init(dev, struct rt_i2c_bus_device *bus, uint16_t addr);
 *   int ds3231_read_time(dev, struct ds3231_time *t);
 *
 * Field mapping (ds3231_time → drivergen_eval_time_t):
 *   ds3231_time.seconds → second
 *   ds3231_time.minutes → minute
 *   ds3231_time.hours   → hour
 *   ds3231_time.day     → weekday    (DS3231 uses 'day' for day-of-week)
 *   ds3231_time.date    → day        (DS3231 uses 'date' for day-of-month)
 *   ds3231_time.month   → month
 *   ds3231_time.year    → year + 2000
 *
 * The reference driver already converts BCD → decimal internally, so the
 * adapter only does the name mapping and the 2000-year base offset.
 *
 * set_time is not supported by this reference driver; the adapter
 * returns DRIVERGEN_EVAL_ERR_UNSUPPORTED. The harness marks this as
 * non-fatal when DRIVERGEN_RTC_DO_SET is not defined (oracle stimuli
 * for DS3231 only cover get_time preload paths).
 */
#include "drivergen_eval_adapter.h"
#include "ds3231_ref.h"

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
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_device_t bus_dev = rt_device_find(bus_name);
    if (bus_dev == RT_NULL) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)bus_dev;
    if (ds3231_init(&g_eval_dev, bus, DS3231_ADDR_DEFAULT) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    return DRIVERGEN_EVAL_OK;
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
