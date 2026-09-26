#include "dht22.h"
#include "ch.h"
#include "hal.h"
#include "hal_pal.h"
#include <stdint.h>
#include <stddef.h>

#define DHT22_TIMEOUT_US 200

static int dht22_wait_for_level(struct dht22_device *dev, bool level, uint32_t timeout_us) {
    uint32_t elapsed = 0;
    while (palReadLine(dev->echo_line) != level) {
        chThdSleepMicroseconds(1);
        elapsed++;
        if (elapsed >= timeout_us) {
            return -1;
        }
    }
    return 0;
}

int dht22_init(struct dht22_device *dev, uint32_t trig_line) {
    dev->trig_line = trig_line;
    dev->echo_line = PAL_LINE(GPIOA, 1);
    dev->bus_handle = NULL;
    palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOA, 1, PAL_MODE_INPUT);
    chThdSleepMilliseconds(1000);
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature) {
    uint8_t data[5] = {0};
    int bit_idx, byte_idx;
    uint32_t pulse_len;
    
    // Send start signal: pull low for at least 18ms
    palClearPad(GPIOA, 0);
    chThdSleepMilliseconds(20);
    palSetPadMode(GPIOA, 0, PAL_MODE_INPUT);
    
    // Wait for sensor response: pull low (80us)
    if (dht22_wait_for_level(dev, false, DHT22_TIMEOUT_US) != 0) {
        palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
        palSetPad(GPIOA, 0);
        return -1;
    }
    // Wait for high (80us)
    if (dht22_wait_for_level(dev, true, DHT22_TIMEOUT_US) != 0) {
        palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
        palSetPad(GPIOA, 0);
        return -1;
    }
    
    // Read 40 bits
    for (byte_idx = 0; byte_idx < 5; byte_idx++) {
        for (bit_idx = 7; bit_idx >= 0; bit_idx--) {
            // Wait for low (50us)
            if (dht22_wait_for_level(dev, false, DHT22_TIMEOUT_US) != 0) {
                palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
                palSetPad(GPIOA, 0);
                return -1;
            }
            // Wait for high, measure pulse length
            uint32_t start = 0;
            while (palReadLine(dev->echo_line) == false) {
                chThdSleepMicroseconds(1);
                start++;
                if (start > DHT22_TIMEOUT_US) {
                    palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
                    palSetPad(GPIOA, 0);
                    return -1;
                }
            }
            pulse_len = 0;
            while (palReadLine(dev->echo_line) == true) {
                chThdSleepMicroseconds(1);
                pulse_len++;
                if (pulse_len > DHT22_TIMEOUT_US) {
                    palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
                    palSetPad(GPIOA, 0);
                    return -1;
                }
            }
            if (pulse_len > 40) {
                data[byte_idx] |= (1 << bit_idx);
            }
        }
    }
    
    // Restore output mode and set high
    palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPad(GPIOA, 0);
    
    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -1;
    }
    
    // Convert humidity: integral_RH * 100 + decimal_RH
    uint16_t integral_rh = (data[0] << 8) | data[1];
    *humidity = (int32_t)integral_rh * 100 + (int32_t)integral_rh;
    
    // Convert temperature: integral_T * 100 + decimal_T
    uint16_t integral_t = (data[2] << 8) | data[3];
    *temperature = (int32_t)integral_t * 100 + (int32_t)integral_t;
    
    return 0;
}