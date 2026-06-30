/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LM75A Temperature Sensor Driver for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          NuttX reference driver
 */
#ifndef __LM75A_REF_H
#define __LM75A_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* LM75A 7-bit I2C address range: 0x48 ~ 0x4F (A2/A1/A0 pins) */
#define LM75A_ADDR_MIN               0x48
#define LM75A_ADDR_MAX               0x4F
#define LM75A_DEFAULT_ADDR           0x48
#define LM75A_I2C_FREQ               100000

/* Register Map */
#define LM75A_REG_TEMP               0x00
#define LM75A_REG_CONF               0x01
#define LM75A_REG_THYST              0x02
#define LM75A_REG_TOS                0x03

/* Configuration Register bits */
#define LM75A_CONF_SHUTDOWN          (1U << 0)
#define LM75A_CONF_OS_COMP_INT       (1U << 1)
#define LM75A_CONF_OS_POLARITY       (1U << 2)
#define LM75A_CONF_FAULT_QUEUE_0     (1U << 3)
#define LM75A_CONF_FAULT_QUEUE_1     (1U << 4)

/* Temperature range in milli-Celsius */
#define LM75A_TEMP_MC_MIN            (-55000)
#define LM75A_TEMP_MC_MAX            125000
#define LM75A_TEMP_STEP_MC           125

struct lm75a_device
{
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
};

int lm75a_init(FAR struct lm75a_device *dev,
               FAR struct i2c_master_s *i2c,
               uint8_t addr);

int lm75a_probe(FAR struct lm75a_device *dev);

int lm75a_read_temperature(FAR struct lm75a_device *dev,
                           FAR int32_t *temp_mcelsius);

int lm75a_read_config(FAR struct lm75a_device *dev,
                      FAR uint8_t *config);

int lm75a_write_config(FAR struct lm75a_device *dev,
                       uint8_t config);

int lm75a_set_shutdown(FAR struct lm75a_device *dev, bool enable);

int lm75a_set_tos(FAR struct lm75a_device *dev,
                  int32_t tos_mcelsius);

int lm75a_set_thyst(FAR struct lm75a_device *dev,
                    int32_t thyst_mcelsius);

#ifdef __cplusplus
}
#endif

/* EVAL_COMPAT_SHIM */

/*
 * Compatibility shim: adapter calls lm75a_read_raw() but this driver
 * implements lm75a_read_temperature(out_mcelsius).
 *
 * Eval ABI expects raw in eighth_celsius (0.125 C/LSB). Divide
 * millicelsius by LM75A_TEMP_STEP_MC=125 (exact, since LM75A quantises
 * to 0.125 C). Range is safe: 125 C = 1000, -55 C = -440, both fit in
 * int16_t.
 */
static inline int lm75a_read_raw(struct lm75a_device *dev, int16_t *raw) {
    int32_t temp_mc = 0;
    int err = lm75a_read_temperature(dev, &temp_mc);
    if (raw) *raw = (int16_t)(temp_mc / LM75A_TEMP_STEP_MC);
    return err;
}


#endif /* __LM75A_REF_H */
