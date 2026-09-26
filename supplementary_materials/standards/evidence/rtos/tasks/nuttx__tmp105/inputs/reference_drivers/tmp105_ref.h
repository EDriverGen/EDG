/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Driver for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          NuttX reference driver
 */
#ifndef __TMP105_REF_H
#define __TMP105_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* TMP105 7-bit I2C address: 0x48 ~ 0x4B */
#define TMP105_ADDR_DEFAULT          0x48
#define TMP105_I2C_FREQ              100000

/* Register Map */
#define TMP105_REG_TEMP              0x00
#define TMP105_REG_CONF              0x01
#define TMP105_REG_TLOW              0x02
#define TMP105_REG_THIGH             0x03

/* Configuration bits */
#define TMP105_CONF_SD               (1U << 0)  /* Shutdown Mode */
#define TMP105_CONF_TM               (1U << 1)  /* Thermostat Mode */
#define TMP105_CONF_POL              (1U << 2)  /* Polarity */
#define TMP105_CONF_F0               (1U << 3)  /* Fault Queue bit 0 */
#define TMP105_CONF_F1               (1U << 4)  /* Fault Queue bit 1 */
#define TMP105_CONF_R0               (1U << 5)  /* Resolution bit 0 */
#define TMP105_CONF_R1               (1U << 6)  /* Resolution bit 1 */
#define TMP105_CONF_OS               (1U << 7)  /* One-Shot */

/* Resolution: 9-bit (0.5C), 10-bit (0.25C), 11-bit (0.125C), 12-bit (0.0625C) */
#define TMP105_RES_9BIT              0
#define TMP105_RES_10BIT             1
#define TMP105_RES_11BIT             2
#define TMP105_RES_12BIT             3

#define TMP105_TEMP_MC_MIN           (-55000)
#define TMP105_TEMP_MC_MAX           128000

struct tmp105_device
{
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
  uint8_t resolution;
};

int tmp105_init(FAR struct tmp105_device *dev,
                FAR struct i2c_master_s *i2c,
                uint8_t addr);

int tmp105_probe(FAR struct tmp105_device *dev);

int tmp105_set_resolution(FAR struct tmp105_device *dev, uint8_t res);

int tmp105_read_temperature(FAR struct tmp105_device *dev,
                            FAR int32_t *temp_mcelsius);

int tmp105_read_config(FAR struct tmp105_device *dev,
                       FAR uint8_t *config);

int tmp105_write_config(FAR struct tmp105_device *dev,
                        uint8_t config);

#ifdef __cplusplus
}
#endif

#endif /* __TMP105_REF_H */
