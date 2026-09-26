#include "pcf8574_ref.h"

int pcf8574_init(struct pcf8574_device *dev, I2C_HandleTypeDef *bus, uint16_t addr)
{
    uint8_t val = 0xFF;

    if (!dev || !bus) return -1;
    dev->bus  = bus;
    dev->addr = addr;

    /* Set all pins HIGH (input mode for quasi-bidirectional ports).
     * POR default is already HIGH; this write ensures known state.
     * PCF8574 has NO registers — pure I2C write of 1 byte to port. */
    return pcf8574_write_port(dev, val);
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val)
{
    if (!dev || !dev->bus || !val) return -1;

    /* PCF8574 has NO registers — pure I2C read, no Mem_Read! */
    return HAL_I2C_Master_Receive(dev->bus, (uint16_t)(dev->addr << 1),
                                  val, 1, 100) == HAL_OK ? 0 : -1;
}

int pcf8574_write_port(struct pcf8574_device *dev, uint8_t val)
{
    if (!dev || !dev->bus) return -1;

    /* PCF8574 has NO registers — pure I2C write, no register address byte! */
    return HAL_I2C_Master_Transmit(dev->bus, (uint16_t)(dev->addr << 1),
                                   &val, 1, 100) == HAL_OK ? 0 : -1;
}

/* Per-pin readers: P7=bit7 ... P0=bit0 */
#define _PIN_READ(dev, bit) do { \
    uint8_t v; \
    if (pcf8574_read_port(dev, &v) != 0) return -1; \
    return (int)((v >> bit) & 1); \
} while(0)

int pcf8574_read_p0(struct pcf8574_device *dev) { _PIN_READ(dev, 0); }
int pcf8574_read_p1(struct pcf8574_device *dev) { _PIN_READ(dev, 1); }
int pcf8574_read_p2(struct pcf8574_device *dev) { _PIN_READ(dev, 2); }
int pcf8574_read_p3(struct pcf8574_device *dev) { _PIN_READ(dev, 3); }
int pcf8574_read_p4(struct pcf8574_device *dev) { _PIN_READ(dev, 4); }
int pcf8574_read_p5(struct pcf8574_device *dev) { _PIN_READ(dev, 5); }
int pcf8574_read_p6(struct pcf8574_device *dev) { _PIN_READ(dev, 6); }
int pcf8574_read_p7(struct pcf8574_device *dev) { _PIN_READ(dev, 7); }
