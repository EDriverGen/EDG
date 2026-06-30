#ifndef __STM32F1XX_HAL_H
#define __STM32F1XX_HAL_H
#include <stdint.h>
/* If tobudos.h is included (which it always should be for tobudos builds),
 * skip all type definitions to avoid conflicting with tobudos.h's own
 * richer STM32 HAL declarations.  stm32f1xx_hal.h is only a fallback
 * for builds that don't pull in tobudos.h. */
#ifndef _TOBUDOS_HAL_DEFINED
#include "tobudos.h"
#endif /* !_TOBUDOS_HAL_DEFINED */
#endif
void HAL_Delay(uint32_t ms);
