/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-15     Lin        refactor BH1750 driver to standard structure
 */
#ifndef DRIVERS_INCLUDE_BH1750_H_
#define DRIVERS_INCLUDE_BH1750_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * BH1750 supports two ADDR pin configurations:
 * 1. Low selects the 7-bit I2C address 0x23
 * 2. High selects the 7-bit I2C address 0x5C
 */
#define BH1750_ADDR_LOW             0x23
#define BH1750_ADDR_HIGH            0x5C

/* Default bus name and address. */
#define BH1750_DEFAULT_BUS_NAME     "i2c1"
#define BH1750_DEFAULT_ADDR         BH1750_ADDR_LOW

/*
 * Supported BH1750 measurement modes.
 * Command values come from the BH1750 datasheet's command table.
 */
enum bh1750_measure_mode
{
    BH1750_CONT_H_RES_MODE   = 0x10, /* Continuous high-resolution mode, 1 lx resolution */
    BH1750_CONT_H_RES_MODE2  = 0x11, /* Continuous high-resolution mode 2, 0.5 lx resolution */
    BH1750_CONT_L_RES_MODE   = 0x13, /* Continuous low-resolution mode, 4 lx resolution */
    BH1750_ONE_H_RES_MODE    = 0x20, /* One-shot high-resolution mode; automatic power-down after measurement */
    BH1750_ONE_H_RES_MODE2   = 0x21, /* One-shot high-resolution mode 2; automatic power-down after measurement */
    BH1750_ONE_L_RES_MODE    = 0x23  /* One-shot low-resolution mode; automatic power-down after measurement */
};

/*
 * BH1750 driver device object.
 * It groups the bus, address, and mode without allocating hardware resources,
 * giving upper layers a consistent interface.
 */
struct bh1750_device
{
    struct rt_i2c_bus_device *bus; /* Bound I2C bus object */
    const char *bus_name;          /* I2C bus name, such as i2c1 */
    rt_uint8_t addr;               /* BH1750 7-bit I2C address */
    rt_uint8_t mode;               /* Current measurement mode */
};

/*
 * Check whether an address is supported by BH1750.
 */
rt_bool_t bh1750_is_valid_address(rt_uint8_t addr);

/*
 * Check whether a mode value is a supported BH1750 measurement mode.
 */
rt_bool_t bh1750_is_valid_mode(rt_uint8_t mode);

/*
 * Convert a mode value to a printable English description.
 * Returns a static string that must not be freed.
 */
const char *bh1750_mode_to_string(rt_uint8_t mode);

/*
 * Initialize a BH1750 device object with default or specified parameters.
 * This function only binds the software object; it sends no I2C commands.
 */
rt_err_t bh1750_init(struct bh1750_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr);

/*
 * Change the device object's measurement mode.
 */
rt_err_t bh1750_set_mode(struct bh1750_device *dev, rt_uint8_t mode);

/*
 * Probe for BH1750 at the specified address.
 * Send a valid command; an ACK indicates that a device is likely online.
 */
rt_err_t bh1750_probe(struct bh1750_device *dev);

/*
 * Read one raw 16-bit BH1750 measurement.
 */
rt_err_t bh1750_read_raw(struct bh1750_device *dev, rt_uint16_t *raw);

/*
 * Convert a BH1750 raw value to lux * 100.
 * For example, 12345 represents 123.45 lux.
 */
rt_uint32_t bh1750_raw_to_lux_x100(rt_uint16_t raw);

/*
 * Read a converted illuminance value directly, in lux * 100.
 */
rt_err_t bh1750_read_lux_x100(struct bh1750_device *dev, rt_uint32_t *lux_x100);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_BH1750_H_ */
