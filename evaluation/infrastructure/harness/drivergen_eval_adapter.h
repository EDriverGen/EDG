/* drivergen_eval_adapter.h
 *
 * Unified Evaluation ABI for DriverGen.
 *
 * This header is the **single contract** between:
 *   1. The generated driver (`<device>.h` + `<device>.c`)
 *   2. The auto-generated adapter (`<device>_eval_adapter.c`,
 *      produced by `drivergen/codegen/adapter_generator.py`)
 *   3. The evaluation harness (`test_main_<eval_class>.c`)
 *   4. The evaluation runtime / ladder judges
 *
 * Every <device>_eval_adapter.c MUST:
 *   1. Define `drivergen_eval_meta` (the global const metadata struct).
 *   2. Implement `drivergen_eval_init(bus_name)` and
 *      `drivergen_eval_cleanup()`.
 *   3. Implement the subset of functions corresponding to its eval_class:
 *        - "single_channel" : drivergen_eval_read_raw_i32
 *        - "multi_channel"  : drivergen_eval_read_channel
 *        - "memory"         : drivergen_eval_mem_read +
 *                             drivergen_eval_mem_write
 *        - "display"        : drivergen_eval_output_frame
 *                             (+ drivergen_eval_read_status if device
 *                              supports it; else return non-zero)
 *        - "rtc"            : drivergen_eval_get_time +
 *                             drivergen_eval_set_time
 *   4. NOT implement functions outside its class. The corresponding
 *      harness template only references the functions for its class,
 *      so unused functions never appear in the link set.
 *
 * Naming convention:
 *   - All public symbols prefixed with `drivergen_eval_`.
 *   - Return code: 0 on success, non-zero on failure. `DRIVERGEN_EVAL_OK`
 *     and named error codes are provided for clarity but NOT required;
 *     a plain `0`/`-1` is fine and the harness only checks `== 0`.
 *
 * Stability promise:
 *   - Adding new fields to `drivergen_eval_meta_t` at the end is
 *     non-breaking (existing adapters stay valid as long as they still
 *     initialize the original fields).
 *   - Adding new eval_class constants is non-breaking.
 *   - Removing or reordering existing function signatures IS breaking and
 *     requires a version bump.
 */
#ifndef DRIVERGEN_EVAL_ADAPTER_H
#define DRIVERGEN_EVAL_ADAPTER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================== *
 *  ABI version                                                           *
 * ====================================================================== */

#define DRIVERGEN_EVAL_ABI_VERSION_MAJOR 1
#define DRIVERGEN_EVAL_ABI_VERSION_MINOR 0

/* ====================================================================== *
 *  Return codes (optional convenience; harness only checks `== 0`)      *
 * ====================================================================== */

#define DRIVERGEN_EVAL_OK             0
#define DRIVERGEN_EVAL_ERR_IO        -1  /* generic I/O failure */
#define DRIVERGEN_EVAL_ERR_INVALID   -2  /* invalid arg / out of range */
#define DRIVERGEN_EVAL_ERR_NACK      -3  /* bus NACK (I2C) */
#define DRIVERGEN_EVAL_ERR_TIMEOUT   -4
#define DRIVERGEN_EVAL_ERR_CRC       -5
#define DRIVERGEN_EVAL_ERR_UNSUPPORTED -6 /* operation not supported by device */

/* ====================================================================== *
 *  eval_class identifiers (string constants, used in metadata)          *
 * ====================================================================== */

#define DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL "single_channel"
#define DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL  "multi_channel"
#define DRIVERGEN_EVAL_CLASS_MEMORY         "memory"
#define DRIVERGEN_EVAL_CLASS_DISPLAY        "display"
#define DRIVERGEN_EVAL_CLASS_RTC            "rtc"

/* ====================================================================== *
 *  Type definitions                                                      *
 * ====================================================================== */

/**
 * Per-channel descriptor used by multi_channel devices.
 *
 * `id`             : human-readable channel identifier
 *                    (e.g. "accel_x", "temp_mc", "humidity_pct").
 * `physical_unit`  : descriptive unit string for oracle interpretation
 *                    (e.g. "lsb_per_g", "milli_celsius",
 *                     "percent_x100"). Free-form; evaluation oracle
 *                    matches against the same strings declared in
 *                    `oracle/<device>/meta.json`.
 * `scale`          : optional integer hint for raw -> physical
 *                    multiplication (0 = identity / not applicable).
 *                    Adapter does NOT apply this; it is metadata for
 *                    oracle/reporting.
 */
typedef struct {
    const char *id;
    const char *physical_unit;
    int32_t     scale;
} drivergen_eval_channel_t;

/**
 * Calendar time structure for RTC devices.
 *
 * Months are 1-based (1..12), days 1-based (1..31). Hours 0..23,
 * minute/second 0..59. `weekday` is 0..6 with 0 = Sunday; set to 0 if
 * the device does not track day-of-week.
 *
 * `reserved` keeps the struct 8-byte aligned; must be set to 0 by writers
 * and ignored by readers.
 */
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  weekday;
    uint8_t  reserved;
} drivergen_eval_time_t;

/**
 * Adapter metadata. Every adapter MUST define exactly one global instance
 * named `drivergen_eval_meta` (extern declared below).
 *
 * Field grouping:
 *   - Always required: device_id, eval_class
 *   - single_channel  : primary_id, primary_unit
 *                       (channel_count = 0, channels = NULL)
 *   - multi_channel   : channel_count, channels
 *                       (primary_* may also be set as a hint, defaulting
 *                       to channels[0])
 *   - memory          : memory_size_bytes, memory_page_bytes
 *   - display, rtc    : no class-specific metadata (rely on eval_class
 *                       string for dispatch)
 */
typedef struct {
    /* Always required */
    const char *device_id;            /* task-package device identifier */
    const char *eval_class;           /* one of DRIVERGEN_EVAL_CLASS_* */

    /* multi_channel (and single_channel as primary fallback) */
    int channel_count;                /* >0 for multi_channel; else 0 */
    const drivergen_eval_channel_t *channels;  /* NULL when count == 0 */

    /* single_channel primary (also used as multi_channel.channels[0] hint) */
    const char *primary_id;
    const char *primary_unit;

    /* memory class */
    uint32_t memory_size_bytes;       /* 0 if not memory class */
    uint16_t memory_page_bytes;       /* 0 if unknown / not enforced */

    /* ABI version (set by adapter_generator.py from this header) */
    uint16_t abi_version_major;
    uint16_t abi_version_minor;
} drivergen_eval_meta_t;

extern const drivergen_eval_meta_t drivergen_eval_meta;

/* ====================================================================== *
 *  Common interface (required by ALL eval_classes)                       *
 * ====================================================================== */

/**
 * Initialize the driver against a named bus instance.
 *
 * `bus_name` : a bus-instance string identifier supplied by the harness.
 *              Format depends on RTOS / bus type:
 *                I2C  : "i2c1", "i2c2", ...
 *                SPI  : "spi1", "spi2", ...
 *                UART : "uart1", "uart2", ...
 *                GPIO : platform-specific pin label, e.g. "PB5"
 *              The harness injects this string at compile time via the
 *              build configuration; the adapter passes it through to the
 *              underlying driver init function unchanged.
 *
 * Returns: 0 on success, non-zero on failure.
 */
int drivergen_eval_init(const char *bus_name);

/**
 * Release any resources allocated by drivergen_eval_init.
 * For most polling drivers this is a no-op returning 0.
 *
 * Returns: 0 on success, non-zero on failure.
 */
int drivergen_eval_cleanup(void);

/* ====================================================================== *
 *  Class: single_channel                                                 *
 * ====================================================================== */

/**
 * Read the device's primary channel as a signed 32-bit raw value.
 *
 * The adapter widens the driver's native return type (e.g. uint8_t,
 * uint16_t, int16_t) to int32_t. Sign-extension is applied for signed
 * source types; zero-extension for unsigned. No unit conversion happens
 * here; the value is the driver's "raw" reading.
 *
 * `out` : non-NULL output pointer.
 *
 * Returns: 0 on success, non-zero on failure.
 */
int drivergen_eval_read_raw_i32(int32_t *out);

/* ====================================================================== *
 *  Class: multi_channel                                                  *
 * ====================================================================== */

/**
 * Read a specific channel by index into drivergen_eval_meta.channels[].
 *
 * `channel_id` : 0..(drivergen_eval_meta.channel_count - 1).
 * `out`        : non-NULL output pointer.
 *
 * Adapters MAY internally cache results from a single underlying read
 * call when the device returns multiple channels in one bus transaction
 * (e.g. accel_x/y/z from MPU6050 ACCEL_XOUT_H). This is invisible to the
 * caller. However, the cache MUST be invalidated each time the harness
 * begins a new logical "read cycle" (typically by calling channel 0).
 *
 * Returns: 0 on success, non-zero on failure (incl. invalid channel_id).
 */
int drivergen_eval_read_channel(int channel_id, int32_t *out);

/* ====================================================================== *
 *  Class: memory                                                         *
 * ====================================================================== */

/**
 * Read `len` bytes starting at device address `addr`.
 *
 * `addr` : device-internal byte address, in [0, memory_size_bytes).
 * `buf`  : caller-allocated output buffer of at least `len` bytes.
 * `len`  : number of bytes to read; SHOULD NOT exceed
 *          memory_page_bytes if the device requires page-aligned reads.
 *          Adapters that cannot enforce page boundaries MAY return
 *          DRIVERGEN_EVAL_ERR_INVALID for cross-page reads.
 *
 * Returns: 0 on success, non-zero on failure.
 */
int drivergen_eval_mem_read(uint32_t addr, uint8_t *buf, uint16_t len);

/**
 * Write `len` bytes starting at device address `addr`.
 *
 * Same address/length semantics as drivergen_eval_mem_read. The adapter
 * SHOULD block until the write cycle completes (typical EEPROM 5-10 ms).
 *
 * Returns: 0 on success, non-zero on failure.
 */
int drivergen_eval_mem_write(uint32_t addr, const uint8_t *buf, uint16_t len);

/* ====================================================================== *
 *  Class: display                                                        *
 * ====================================================================== */

/**
 * Send one frame (or sub-frame) of display data to the device.
 *
 * `data` / `len` : raw byte payload as the device expects, including any
 *                  position/control prefix bytes. The adapter passes this
 *                  through to the driver's frame-write function.
 *
 * Returns: 0 on success, non-zero on failure.
 */
int drivergen_eval_output_frame(const uint8_t *data, uint16_t len);

/**
 * Read a status byte from the device (display-controllers that support it).
 *
 * `out` : non-NULL output pointer.
 *
 * Returns: 0 on success, non-zero on failure.
 *          Devices that do not support status read MUST return
 *          DRIVERGEN_EVAL_ERR_UNSUPPORTED (-6).
 */
int drivergen_eval_read_status(uint8_t *out);

/* ====================================================================== *
 *  Class: rtc                                                            *
 * ====================================================================== */

/**
 * Read the current time from the RTC.
 *
 * `out` : non-NULL output pointer; filled with current time on success.
 *
 * Returns: 0 on success, non-zero on failure.
 */
int drivergen_eval_get_time(drivergen_eval_time_t *out);

/**
 * Set the RTC's time.
 *
 * `in`  : non-NULL input pointer with the time to set.
 *
 * Returns: 0 on success, non-zero on failure.
 */
int drivergen_eval_set_time(const drivergen_eval_time_t *in);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERGEN_EVAL_ADAPTER_H */
