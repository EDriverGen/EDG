/* RIOT stub: periph/timer.h — low-level timer peripheral API.
 *
 * Real source: drivers/include/periph/timer.h in RIOT. This stub provides
 * just enough of the public surface (types + prototypes) so drivers that
 * use HC-SR04-style "measure pulse width via GPT" compile and link. The
 * stubs themselves are no-ops defined in stubs_gpio.c / stubs.c if needed,
 * or the compiler can resolve them to weak aliases via -u tricks at a
 * later date. For now we ship the types + declarations; if a driver tries
 * to actually call ``timer_read()`` the link step fails with a clear
 * undefined-symbol error instead of a cryptic header-not-found.
 */
#ifndef DRIVERGEN_STUB_RIOT_PERIPH_TIMER_H
#define DRIVERGEN_STUB_RIOT_PERIPH_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int tim_t;
typedef void (*timer_cb_t)(void *arg, int channel);

#ifndef TIMER_DEV
#define TIMER_DEV(x)    ((tim_t)(x))
#endif

#ifndef TIMER_FLAG_RESET_ON_MATCH
#define TIMER_FLAG_RESET_ON_MATCH   (1u << 0)
#endif
#ifndef TIMER_FLAG_RESET_ON_SET
#define TIMER_FLAG_RESET_ON_SET     (1u << 1)
#endif

int  timer_init(tim_t dev, uint32_t freq, timer_cb_t cb, void *arg);
int  timer_set(tim_t dev, int channel, unsigned int timeout);
int  timer_set_absolute(tim_t dev, int channel, unsigned int value);
int  timer_set_periodic(tim_t dev, int channel, unsigned int value, uint8_t flags);
int  timer_clear(tim_t dev, int channel);
unsigned int timer_read(tim_t dev);
void timer_start(tim_t dev);
void timer_stop(tim_t dev);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERGEN_STUB_RIOT_PERIPH_TIMER_H */
