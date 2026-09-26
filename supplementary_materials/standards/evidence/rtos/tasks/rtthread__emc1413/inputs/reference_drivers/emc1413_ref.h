/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add EMC1413 driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_EMC1413_H_
#define DRIVERS_INCLUDE_EMC1413_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * EMC1413 7-bit I2C address.
 * The default address depends on the package variant; 0x4C is common.
 */
#define EMC1413_ADDR_DEFAULT         0x4C

#define EMC1413_DEFAULT_BUS_NAME     "i2c1"
#define EMC1413_DEFAULT_ADDR         EMC1413_ADDR_DEFAULT

/*
 * EMC1413 register definitions (datasheet Table 4-1).
 */
#define EMC1413_REG_INTERNAL_TEMP_HI 0x00  /* Internal temperature high byte */
#define EMC1413_REG_EXT1_TEMP_HI     0x01  /* External channel 1 temperature high byte */
#define EMC1413_REG_EXT2_TEMP_HI     0x23  /* External channel 2 temperature high byte */
#define EMC1413_REG_STATUS           0x02  /* Status register */
#define EMC1413_REG_CONFIG           0x03  /* Configuration register */
#define EMC1413_REG_CONV_RATE        0x04  /* Conversion rate */
#define EMC1413_REG_INTERNAL_TEMP_LO 0x29  /* Internal temperature low byte */
#define EMC1413_REG_EXT1_TEMP_LO     0x10  /* External channel 1 temperature low byte */
#define EMC1413_REG_EXT2_TEMP_LO     0x24  /* External channel 2 temperature low byte */
#define EMC1413_REG_PRODUCT_ID       0xFD  /* Product ID */
#define EMC1413_REG_MANUFACTURER_ID  0xFE  /* Manufacturer ID: 0x5D (SMSC/Microchip) */
#define EMC1413_REG_REVISION         0xFF  /* Revision number */

/* Manufacturer ID */
#define EMC1413_MANUFACTURER_ID      0x5D

/* Product ID (EMC1413 = 0x21, EMC1414 = 0x25) */
#define EMC1413_PRODUCT_ID           0x21
#define EMC1414_PRODUCT_ID           0x25

/*
 * Configuration-register bit definitions.
 */
#define EMC1413_CONFIG_MASK          (1U << 7) /* 1: Mask ALERT */
#define EMC1413_CONFIG_RUN_STOP      (1U << 6) /* 0: Running, 1: Standby */
#define EMC1413_CONFIG_RANGE         (1U << 2) /* 0: 0~127C, 1: -64~191C */

/*
 * Temperature-channel enumeration.
 */
enum emc1413_channel
{
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};

/*
 * EMC1413 driver device object.
 */
struct emc1413_device
{
    struct rt_i2c_bus_device *bus;
    const char *bus_name;
    rt_uint8_t addr;
};

/*
 * Initialize an EMC1413 device object.
 */
rt_err_t emc1413_init(struct emc1413_device *dev,
                      const char            *bus_name,
                      rt_uint8_t             addr);

/*
 * Probe EMC1413 by checking the manufacturer ID.
 */
rt_err_t emc1413_probe(struct emc1413_device *dev);

/*
 * Read the specified channel's temperature in millidegrees Celsius.
 */
rt_err_t emc1413_read_temperature(struct emc1413_device *dev,
                                  enum emc1413_channel   channel,
                                  rt_int32_t            *temp_mcelsius);

/*
 * Set the extended temperature range (-64~191 C).
 */
rt_err_t emc1413_set_extended_range(struct emc1413_device *dev,
                                    rt_bool_t              enable);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_EMC1413_H_ */
