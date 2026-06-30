#ifndef __LM75A_REF_H
#define __LM75A_REF_H

#include "tos_k.h"
#ifndef HAL_I2C_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#endif
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define LM75A_DEFAULT_ADDR  0x48
#define LM75A_REG_TEMP      0x00
#define LM75A_REG_CONF      0x01

struct lm75a_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
};

int lm75a_init(struct lm75a_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int lm75a_probe(struct lm75a_device *dev);
int lm75a_read_temperature(struct lm75a_device *dev, int32_t *temp_mcelsius);

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


#endif
