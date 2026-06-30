/* projdefs.h — forwarder to the unified FreeRTOS stub.
 *
 * Upstream FreeRTOS splits `pdTRUE` / `pdPASS` / `BaseType_t` / etc.
 * into this header and `FreeRTOS.h` includes it transitively.  Our
 * unified stub body already defines every one of those symbols in
 * `freertos.h`, so this file just forwards to keep
 * `#include <projdefs.h>` happy wherever drivers reach for it
 * directly (STM32Cube BSPs do this on occasion). */
#ifndef __FREERTOS_STUB_PROJDEFS_H__
#define __FREERTOS_STUB_PROJDEFS_H__
#include "freertos.h"
#endif
