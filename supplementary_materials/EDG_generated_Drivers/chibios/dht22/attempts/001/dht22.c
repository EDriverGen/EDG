#include "dht22.h"
#include "ch.h"
#include "hal.h"
#include <stddef.h>

#include "hal_pal.h"
#define DHT22_START_SIGNAL_LOW_MS 18
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

void dht22_init(struct dht22_device *dev, uint32_t line) {
    dev->data_line = line;
    palSetPadMode(PAL_PORT(line), PAL_PAD(line), PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(PAL_PORT(line), PAL_PAD(line));
    chThdSleepMilliseconds(1000);
}

static int dht22_wait_for_level(struct dht22_device *dev, bool level, uint32_t timeout_us) {
    uint32_t elapsed = 0;
    while (palReadLine(dev->data_line) != level) {
        chThdSleepMicroseconds(1);
        elapsed++;
        if (elapsed >= timeout_us) return -1;
    }
    return 0;
}

static int dht22_measure_pulse(struct dht22_device *dev, bool level, uint32_t *width_us, uint32_t timeout_us) {
    uint32_t count = 0;
    while (palReadLine(dev->data_line) == level) {
        chThdSleepMicroseconds(1);
        count++;
        if (count >= timeout_us) return -1;
    }
    *width_us = count;
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature) {
    uint32_t line = dev->data_line;
    uint8_t data[5] = {0};
    int bit_idx;
    int byte_idx;
    uint32_t pulse_width;

    // Send start signal: pull low for at least 18ms
    palSetPadMode(PAL_PORT(line), PAL_PAD(line), PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(PAL_PORT(line), PAL_PAD(line));
    chThdSleepMilliseconds(DHT22_START_SIGNAL_LOW_MS);
    palSetPad(PAL_PORT(line), PAL_PAD(line));
    chThdSleepMicroseconds(DHT22_RESPONSE_WAIT_US);

    // Switch to input
    palSetPadMode(PAL_PORT(line), PAL_PAD(line), PAL_MODE_INPUT);

    // Wait for sensor response: low for 80us
    if (dht22_wait_for_level(dev, false, DHT22_TIMEOUT_US) != 0) return -1;
    if (dht22_measure_pulse(dev, false, &pulse_width, DHT22_RESPONSE_LOW_US + 50) != 0) return -1;
    // Wait for high for 80us
    if (dht22_wait_for_level(dev, true, DHT22_TIMEOUT_US) != 0) return -1;
    if (dht22_measure_pulse(dev, true, &pulse_width, DHT22_RESPONSE_HIGH_US + 50) != 0) return -1;

    // Read 40 bits
    for (bit_idx = 0; bit_idx < 40; bit_idx++) {
        // Wait for low (50us)
        if (dht22_wait_for_level(dev, false, DHT22_TIMEOUT_US) != 0) return -1;
        // Wait for high and measure
        if (dht22_wait_for_level(dev, true, DHT22_TIMEOUT_US) != 0) return -1;
        if (dht22_measure_pulse(dev, true, &pulse_width, DHT22_TIMEOUT_US) != 0) return -1;
        // Determine bit: if pulse > 50us, it's 1; else 0
        if (pulse_width > 50) {
            data[bit_idx / 8] |= (1 << (7 - (bit_idx % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;

    // Convert to milli units
    uint16_t integral_rh = ((uint16_t)data[0] << 8) | data[1];
    uint16_t integral_t = ((uint16_t)data[2] << 8) | data[3];
    *humidity = (int32_t)integral_rh * 100 + (int32_t)data[1];
    *temperature = (int32_t)integral_t * 100 + (int32_t)data[3];

    return 0;
}
