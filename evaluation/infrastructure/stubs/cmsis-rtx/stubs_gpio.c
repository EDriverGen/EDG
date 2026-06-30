#include "cmsis_rtx.h"
#include "hw_uart.h"
#include <stdarg.h>
#include <stdio.h>

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018)
#define GPIOA_BASE  0x40010800U
#define GPIOB_BASE  0x40010C00U
#define GPIOC_BASE  0x40011000U
#define GPIOD_BASE  0x40011400U
#define GPIO_CRL(b) (*(volatile uint32_t *)((b) + 0x00U))
#define GPIO_CRH(b) (*(volatile uint32_t *)((b) + 0x04U))
#define GPIO_IDR(b) (*(volatile uint32_t *)((b) + 0x08U))
#define GPIO_BSRR(b) (*(volatile uint32_t *)((b) + 0x10U))

GPIO_TypeDef *GPIOA = (GPIO_TypeDef *)GPIOA_BASE;
GPIO_TypeDef *GPIOB = (GPIO_TypeDef *)GPIOB_BASE;
GPIO_TypeDef *GPIOC = (GPIO_TypeDef *)GPIOC_BASE;
GPIO_TypeDef *GPIOD = (GPIO_TypeDef *)GPIOD_BASE;

static uint32_t gpio_base(GPIO_TypeDef *port)
{
    return (uint32_t)(uintptr_t)port;
}

static void gpio_set_pin_mode(uint32_t base, int pin, uint32_t mode)
{
    int shift = (pin < 8 ? pin : pin - 8) * 4;
    volatile uint32_t *reg = pin < 8 ? &GPIO_CRL(base) : &GPIO_CRH(base);
    uint32_t v = *reg;
    v &= ~(0xFU << shift);
    v |= ((mode == GPIO_MODE_INPUT ? 0x4U : 0x1U) << shift);
    *reg = v;
}

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
    uint32_t base = gpio_base(GPIOx);
    if (base == GPIOA_BASE) RCC_APB2ENR |= (1U << 2);
    else if (base == GPIOB_BASE) RCC_APB2ENR |= (1U << 3);
    else if (base == GPIOC_BASE) RCC_APB2ENR |= (1U << 4);
    else if (base == GPIOD_BASE) RCC_APB2ENR |= (1U << 5);
    if (base == 0 || GPIO_Init == 0) {
        return;
    }
    for (int pin = 0; pin < 16; pin++) {
        if (GPIO_Init->Pin & (1U << pin)) {
            gpio_set_pin_mode(base, pin, GPIO_Init->Mode);
        }
    }
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    uint32_t base = gpio_base(GPIOx);
    if (base == 0) {
        return;
    }
    if (PinState == GPIO_PIN_SET) {
        GPIO_BSRR(base) = GPIO_Pin;
    } else {
        GPIO_BSRR(base) = ((uint32_t)GPIO_Pin << 16);
    }
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint32_t base = gpio_base(GPIOx);
    if (base == 0) {
        return GPIO_PIN_RESET;
    }
    return (GPIO_IDR(base) & GPIO_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_PinState s = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, s == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

osStatus_t osDelay(uint32_t ticks) { (void)ticks; return osOK; }
uint32_t osKernelGetTickCount(void) { static uint32_t t; return t += 10; }
uint32_t osKernelGetTickFreq(void) { return 1000; }
osMutexId_t osMutexNew(const void *attr) { (void)attr; return (void *)1; }
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout) { (void)mutex_id; (void)timeout; return osOK; }
osStatus_t osMutexRelease(osMutexId_t mutex_id) { (void)mutex_id; return osOK; }
osStatus_t osMutexDelete(osMutexId_t mutex_id) { (void)mutex_id; return osOK; }
void HAL_Delay(uint32_t Delay) { (void)Delay; }
uint32_t HAL_GetTick(void) { return osKernelGetTickCount(); }
HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

int printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) {
        hw_uart2_putc(buf[i]);
    }
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
