#include "max31855_ref.h"
#include "ch.h"
#include "chprintf.h"

static const SPIConfig spicfg = { .circular=false, .end_cb=NULL,
    .ssport=GPIOA, .sspad=4u, .cr1=0, .cr2=0 };

void max31855_sample(BaseSequentialStream *chp) {
    struct max31855_device tc;
    max31855_init(&tc, &SPID1, &spicfg);
    for (int i = 0; i < 5; i++) {
        int32_t t; if (max31855_read_thermocouple(&tc, &t) == 0)
            chprintf(chp, "TC: %d.%03d C\r\n", t/1000, (t>=0?t:-t)%1000);
        chThdSleepMilliseconds(1000);
    }
}
