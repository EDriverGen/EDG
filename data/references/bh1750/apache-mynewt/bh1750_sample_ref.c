#include "bh1750_ref.h"

int bh1750_sample_read(uint16_t *raw)
{
    struct bh1750_device dev;
    if (bh1750_init(&dev, 0, BH1750_DEFAULT_ADDR) != 0) {
        return -1;
    }
    return bh1750_read_raw(&dev, raw);
}
