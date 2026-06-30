/* stm32f1xx_hal_def.h — stub forwarder.
 *
 * Provenance (verified 2026-04-26):
 *   real upstream file is shipped by STMicroelectronics in
 *   STM32CubeF1XX HAL.  In our RTOS corpus it lives at:
 *     data/rtos/threadx-stm32f103-project/STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_def.h
 *
 *   The upstream header defines HAL common types/macros that drivers
 *   transitively pick up via #include "stm32f1xx_hal.h":
 *     - HAL_StatusTypeDef enum (HAL_OK / HAL_ERROR / HAL_BUSY / HAL_TIMEOUT)
 *     - HAL_LockTypeDef enum
 *     - HAL_MAX_DELAY macro
 *     - HAL_IS_BIT_SET / HAL_IS_BIT_CLR / __HAL_LINKDMA / UNUSED macros
 *
 * In our stub bundle these symbols are emitted by the per-RTOS unified
 * stub body (freertos.h / threadx_stub.h / cmsis_os2.h-style)
 * which is transitively included by stm32f1xx_hal.h.  This file
 * is a single-line forwarder so a generated driver that follows the
 * upstream STM32 SDK convention of #include "stm32f1xx_hal_def.h"
 * (which is the canonical way to obtain HAL_StatusTypeDef / HAL_OK)
 * compiles against our stub linker without modification.
 *
 * Why not vendor the real upstream file?  We deliberately keep the
 * stub bundle minimal — it must compile/link with arm-gcc but is never
 * executed.  The unified stub body in stm32f1xx_hal.h already
 * supplies functional declarations for HAL_StatusTypeDef + the HAL_*
 * enums; duplicating them here would risk double-definition errors
 * across translation units.
 *
 */
#ifndef __STM32F1XX_HAL_DEF_STUB_H__
#define __STM32F1XX_HAL_DEF_STUB_H__
#include "stm32f1xx_hal.h"
#endif
