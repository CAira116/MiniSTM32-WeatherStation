#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

/* MiniSTM32 V3.0 板载 LED 引脚定义 */
#define LED0_PORT   GPIOE
#define LED0_PIN    GPIO_Pin_5   // DS0 红色 LED, PE5
#define LED1_PORT   GPIOB
#define LED1_PIN    GPIO_Pin_5   // DS1 绿色 LED, PB5

/* 板载 LED 低电平点亮 */
#define LED0_ON()   GPIO_ResetBits(LED0_PORT, LED0_PIN)
#define LED0_OFF()  GPIO_SetBits(LED0_PORT, LED0_PIN)
#define LED1_ON()   GPIO_ResetBits(LED1_PORT, LED1_PIN)
#define LED1_OFF()  GPIO_SetBits(LED1_PORT, LED1_PIN)

/* 翻转 LED */
#define LED0_TOGGLE() (LED0_PORT->ODR ^= LED0_PIN)
#define LED1_TOGGLE() (LED1_PORT->ODR ^= LED1_PIN)

void LED_Init(void);

#endif
