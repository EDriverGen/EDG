#include "bh1750_ref.h"

extern I2C_HandleTypeDef hi2c1;

int bh1750_sample_read(uint16_t *raw)
{
    struct bh1750_device dev;
    if (bh1750_init(&dev, &hi2c1, BH1750_DEFAULT_ADDR) != 0) {
        return -1;
    }
    return bh1750_read_raw(&dev, raw);
}
