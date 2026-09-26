/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * LM75A Temperature Sensor Driver for RIOT OS
 */
#ifndef __LM75A_REF_H
#define __LM75A_REF_H

#include "periph/i2c.h"
#include "ztimer.h"
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LM75A_ADDR_MIN               0x48
#define LM75A_ADDR_MAX               0x4F
#define LM75A_DEFAULT_ADDR           0x48

#define LM75A_REG_TEMP               0x00
#define LM75A_REG_CONF               0x01
#define LM75A_REG_THYST              0x02
#define LM75A_REG_TOS                0x03

#define LM75A_CONF_SHUTDOWN          (1U << 0)

struct lm75a_device
{
    i2c_t bus;          /* RIOT I2C device index */
    uint16_t addr;
};

int lm75a_init(struct lm75a_device *dev, i2c_t bus, uint16_t addr);
int lm75a_probe(struct lm75a_device *dev);
int lm75a_read_temperature(struct lm75a_device *dev, int32_t *temp_mcelsius);
int lm75a_read_config(struct lm75a_device *dev, uint8_t *config);
int lm75a_write_config(struct lm75a_device *dev, uint8_t config);
int lm75a_set_shutdown(struct lm75a_device *dev, bool enable);

#ifdef __cplusplus
}
#endif

/* EVAL_COMPAT_SHIM */

/*
 * Compatibility shim: adapter calls lm75a_read_raw() but this driver
 * implements lm75a_read_temperature(out_mcelsius).
 *
 * Eval ABI expects raw in eighth_celsius (0.125 C/LSB). Divide
 * millicelsius by 125 (exact for LM75A, which quantises to 0.125 C).
 * Range is safe: 125 C = 1000, -55 C = -440, both fit in int16_t.
 */
static inline int lm75a_read_raw(struct lm75a_device *dev, int16_t *raw) {
    int32_t temp_mc = 0;
    int err = lm75a_read_temperature(dev, &temp_mc);
    if (raw) *raw = (int16_t)(temp_mc / 125);
    return err;
}


#endif /* __LM75A_REF_H */
