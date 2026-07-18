#ifndef __RCC_H
#define __RCC_H

#include "stm32f10x.h"

void rcc_enable(GPIO_TypeDef *port);
void rcc_sysclk_init(void);

// 外设时钟宏
#define PERIPH_USART1  0
#define PERIPH_USART2  1
#define PERIPH_TIM2    2
void rcc_enable_periph(uint8_t periph);

#endif
