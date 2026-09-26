/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-15     Lin          add LM75A driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_LM75A_H_
#define DRIVERS_INCLUDE_LM75A_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * LM75A uses the following 7-bit I2C address format:
 * 1001 A2 A1 A0
 *
 * The A2/A1/A0 connections select addresses from 0x48 to 0x4F.
 * The module's example code uses 8-bit addresses 0x90/0x91,
 * which correspond to 7-bit address 0x48; this is the default here.
 */
#define LM75A_ADDR_MIN               0x48
#define LM75A_ADDR_MAX               0x4F

/* Default bus name and address. */
#define LM75A_DEFAULT_BUS_NAME       "i2c1"
#define LM75A_DEFAULT_ADDR           LM75A_ADDR_MIN

/*
 * LM75A register-pointer values.
 * Write the register number before the actual data read or write.
 */
#define LM75A_REG_TEMP               0x00
#define LM75A_REG_CONF               0x01
#define LM75A_REG_THYST              0x02
#define LM75A_REG_TOS                0x03

/*
 * Configuration Register bit definitions.
 * Bit meanings follow the datasheet's configuration-register description.
 */
#define LM75A_CONF_SHUTDOWN          (1U << 0) /* 1: Enter shutdown mode */
#define LM75A_CONF_OS_COMP_INT       (1U << 1) /* 0: comparator, 1: interrupt */
#define LM75A_CONF_OS_POLARITY       (1U << 2) /* 0: OS active low, 1: OS active high */
#define LM75A_CONF_FAULT_QUEUE_0     (1U << 3)
#define LM75A_CONF_FAULT_QUEUE_1     (1U << 4)
#define LM75A_CONF_OS_OPERATION      (1U << 5)

/*
 * LM75A's standard-mode temperature range is approximately -55 C to +125 C.
 * Millidegrees Celsius are used for convenient integer arithmetic on the MCU:
 * 25000 represents 25.000 C, and -125 represents -0.125 C.
 */
#define LM75A_TEMP_MC_MIN            (-55000)
#define LM75A_TEMP_MC_MAX            125000

/*
 * LM75A temperature resolution is 0.125 C, or 125 mC.
 * The driver defines a raw value as a signed integer in units of 0.125 C.
 */
#define LM75A_TEMP_STEP_MC           125

/*
 * LM75A driver device object.
 * It stores software bindings such as the bus and address,
 * without owning hardware resources or requiring separate dynamic allocation.
 */
struct lm75a_device
{
    struct rt_i2c_bus_device *bus; /* Bound I2C bus object */
    const char *bus_name;          /* I2C bus name, such as i2c1 */
    rt_uint8_t addr;               /* LM75A 7-bit I2C address */
};

/*
 * Check whether an address is within the LM75A supported range.
 */
rt_bool_t lm75a_is_valid_address(rt_uint8_t addr);

/*
 * Initialize an LM75A device object.
 * This function finds the bus and saves parameters without immediately accessing the chip.
 */
rt_err_t lm75a_init(struct lm75a_device *dev,
                    const char          *bus_name,
                    rt_uint8_t           addr);

/*
 * Probe for LM75A at the specified address.
 * Read the Configuration Register;
 * a normal ACK and one returned data byte indicate that a device is likely present.
 */
rt_err_t lm75a_probe(struct lm75a_device *dev);

/*
 * Read and write the Configuration Register.
 */
rt_err_t lm75a_read_config(struct lm75a_device *dev, rt_uint8_t *config);
rt_err_t lm75a_write_config(struct lm75a_device *dev, rt_uint8_t config);

/*
 * Convenience API: enable or disable shutdown mode.
 * Performs a read-modify-write of the configuration register.
 */
rt_err_t lm75a_set_shutdown(struct lm75a_device *dev, rt_bool_t enable);

/*
 * Read one raw value from the temperature register.
 * The returned raw value is in units of 0.125 C.
 * Examples:
 * raw = 200  represents 25.000 C
 * raw = -1   represents -0.125 C
 */
rt_err_t lm75a_read_raw(struct lm75a_device *dev, rt_int16_t *raw);

/*
 * Convert the raw value to millidegrees Celsius.
 * For example, raw = 200 returns 25000.
 */
rt_int32_t lm75a_raw_to_mcelsius(rt_int16_t raw);

/*
 * Read a converted temperature directly, in millidegrees Celsius.
 */
rt_err_t lm75a_read_temp_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius);

/*
 * Read/write T_HYST and T_OS thresholds in millidegrees Celsius.
 * Values that are not multiples of 0.125 C are rounded to the nearest representable value on write.
 */
rt_err_t lm75a_read_thyst_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius);
rt_err_t lm75a_write_thyst_mcelsius(struct lm75a_device *dev, rt_int32_t temp_mcelsius);
rt_err_t lm75a_read_tos_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius);
rt_err_t lm75a_write_tos_mcelsius(struct lm75a_device *dev, rt_int32_t temp_mcelsius);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_LM75A_H_ */
