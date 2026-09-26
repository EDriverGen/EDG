#include "dht22.h"
#include "ch.h"
#include "hal.h"
#include "hal_pal.h"
#include <stdint.h>
#include <stddef.h>

#define DHT22_TIMEOUT_US 200

static int dht22_wait_for_level(struct dht22_device *dev, bool level, uint32_t timeout_us) {
    uint32_t elapsed = 0;
    while (elapsed < timeout_us) {
        if (palReadLine(dev->echo_line) == (level ? PAL_HIGH : PAL_LOW)) {
            return 0;
        }
        chThdSleepMicroseconds(1);
        elapsed++;
    }
    return -1;
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

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    uint8_t data[5] = {0};
    int bit_index, byte_index;
    uint32_t pulse_width;
    int ret;

    // Send start signal: pull low for at least 18ms
    palClearPad(GPIOA, 0);
    chThdSleepMilliseconds(20);
    palSetPadMode(GPIOA, 0, PAL_MODE_INPUT); // release line, switch to input

    // Wait for sensor response: low for 80us
    if (dht22_wait_for_level(dev, PAL_LOW, 200) != 0) {
        return -1;
    }
    // Wait for high for 80us
    if (dht22_wait_for_level(dev, PAL_HIGH, 200) != 0) {
        return -1;
    }

    // Read 40 bits
    for (bit_index = 0; bit_index < 40; bit_index++) {
        // Wait for low (50us)
        if (dht22_wait_for_level(dev, PAL_LOW, 200) != 0) {
            return -1;
        }
        // Measure high pulse width
        uint32_t count = 0;
        while (palReadLine(dev->echo_line) == PAL_HIGH && count < 200) {
            chThdSleepMicroseconds(1);
            count++;
        }
        if (count >= 200) {
            return -1;
        }
        // Determine bit: if high pulse > 30us (threshold between 28 and 70), it's 1
        if (count > 30) {
            byte_index = bit_index / 8;
            data[byte_index] |= (1 << (7 - (bit_index % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -1;
    }

    // Convert to milli units
    int32_t integral_rh = data[0];
    int32_t decimal_rh = data[1];
    int32_t integral_t = data[2];
    int32_t decimal_t = data[3];

    *humidity_val = integral_rh * 100 + decimal_rh;
    *temp_val = integral_t * 100 + decimal_t;

    return 0;
}