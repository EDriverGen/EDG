/* Weak fallbacks for the stub-compile sandbox. */

#include "delay.h"
__attribute__((weak)) int usleep(unsigned int usec) { (void)usec; return 0; }
__attribute__((weak)) unsigned int sleep(unsigned int seconds) { (void)seconds; return 0; }
__attribute__((weak)) unsigned int msleep(unsigned int msec) { (void)msec; return 0; }
