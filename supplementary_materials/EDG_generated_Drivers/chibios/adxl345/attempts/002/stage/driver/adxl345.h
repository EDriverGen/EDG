#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>

#define ADXL345_I2C_ADDR 0x53

typedef struct {
    void *bus_handle;
    uint8_t i2c_addr;
} ADXL345Device;

void adxl345_init(ADXL345Device *dev, void *bus_handle);
void adxl345_read_xyz(ADXL345Device *dev, int16_t *ax, int16_t *ay, int16_t *az);

#endif