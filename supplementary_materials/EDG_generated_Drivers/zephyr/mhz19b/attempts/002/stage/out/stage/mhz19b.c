#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <stdint.h>
#include <stddef.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9
#define MHZ19B_START_BYTE 0xFF
#define MHZ19B_CMD_BYTE_INDEX 1
#define MHZ19B_CO2_HIGH_INDEX 2
#define MHZ19B_CO2_LOW_INDEX 3
#define MHZ19B_CHECKSUM_INDEX 8

static uint8_t mhz19b_calculate_checksum(const uint8_t *data, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++) {
		sum += data[i];
	}
	return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(const struct device *dev)
{
	(void)dev;
	return 0;
}

int mhz19b_read_co2(const struct device *dev, int32_t *raw)
{
	if (dev == NULL || raw == NULL) {
		return -1;
	}

	uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
	uint8_t resp[MHZ19B_RESPONSE_LENGTH];

	/* Send command */
	for (int i = 0; i < (int)sizeof(cmd); i++) {
		uart_poll_out(dev, cmd[i]);
	}

	/* Wait for response (simple delay) */
	k_sleep(K_MSEC(100));

	/* Read response */
	int ret = uart_fifo_read(dev, resp, MHZ19B_RESPONSE_LENGTH);
	if (ret != MHZ19B_RESPONSE_LENGTH) {
		return -1;
	}

	/* Validate start byte */
	if (resp[0] != MHZ19B_START_BYTE) {
		return -1;
	}

	/* Validate command byte */
	if (resp[1] != MHZ19B_CMD_READ_CO2) {
		return -1;
	}

	/* Validate checksum */
	uint8_t calc_checksum = mhz19b_calculate_checksum(resp, 8);
	if (calc_checksum != resp[MHZ19B_CHECKSUM_INDEX]) {
		return -1;
	}

	/* Extract CO2 concentration */
	*raw = (int32_t)((uint16_t)resp[MHZ19B_CO2_HIGH_INDEX] * 256 + (uint16_t)resp[MHZ19B_CO2_LOW_INDEX]);

	return 0;
}