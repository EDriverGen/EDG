#include "mhz19b.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9

static int mhz19b_send_command(struct mhz19b_device *dev, const uint8_t *cmd, size_t len)
{
    (void)dev;
    msg_t status = chnWrite((BaseSequentialStream *)&SD2, cmd, len);
    return (status == len) ? 0 : -1;
}

static int mhz19b_read_response(struct mhz19b_device *dev, uint8_t *buf, size_t len)
{
    (void)dev;
    msg_t status = chnRead((BaseSequentialStream *)&SD2, buf, len);
    return (status == len) ? 0 : -1;
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw)
{
    if (!dev || !raw) return -1;

    uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    if (mhz19b_send_command(dev, cmd, sizeof(cmd)) != 0) {
        return -1;
    }

    uint8_t resp[MHZ19B_RESPONSE_LENGTH];
    if (mhz19b_read_response(dev, resp, MHZ19B_RESPONSE_LENGTH) != 0) {
        return -1;
    }

    if (resp[0] != 0xFF || resp[1] != 0x86) {
        return -1;
    }

    uint8_t checksum = 0;
    for (int i = 1; i < 8; i++) {
        checksum += resp[i];
    }
    checksum = (~checksum) + 1;
    if (checksum != resp[8]) {
        return -1;
    }

    *raw = (int32_t)((uint16_t)resp[2] << 8 | resp[3]);
    return 0;
}