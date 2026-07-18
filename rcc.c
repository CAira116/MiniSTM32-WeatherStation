#include "rcc.h"

void rcc_sysclk_init(void) {
    FLASH->ACR |= (0x2 << 0);
    RCC->CR |= (1 << 16);
    while (!(RCC->CR & (1 << 17))) {}
    RCC->CFGR |= (1 << 16);
    RCC->CFGR &= ~(1 << 17);
    RCC->CFGR |= (0x7 << 18);
    RCC->CR |= (1 << 24);
    while (!(RCC->CR & (1 << 25))) {}
    RCC->CFGR |= (0x4 << 8);
    RCC->CFGR |= (0x2 << 0);
    while ((RCC->CFGR & (0xC)) != (0x8)) {}
}

void rcc_enable(GPIO_TypeDef *port) {
    if (port == GPIOA) {
        RCC->APB2ENR |= (1 << 2);
    } else if (port == GPIOB) {
        RCC->APB2ENR |= (1 << 3);
    } else if (port == GPIOC) {
        RCC->APB2ENR |= (1 << 4);
    }
}

// 开外设时钟
void rcc_enable_periph(uint8_t periph) {
    switch (periph) {
        case PERIPH_USART1: RCC->APB2ENR |= (1 << 14); break;
        case PERIPH_USART2: RCC->APB1ENR |= (1 << 17); break;  // USART2 在 APB1
        case PERIPH_TIM2:   RCC->APB1ENR |= (1 << 0);  break;  // TIM2 在 APB1
    }
}

void SystemInit(void){}