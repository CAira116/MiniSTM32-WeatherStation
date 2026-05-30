#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/* 按键引脚 */
#define KEY0_PORT   GPIOE
#define KEY0_PIN    GPIO_Pin_4   // PE4, 按下为低
#define KEY1_PORT   GPIOE
#define KEY1_PIN    GPIO_Pin_3   // PE3, 按下为低
#define WKUP_PORT   GPIOA
#define WKUP_PIN    GPIO_Pin_0   // PA0, 按下为高

/* 读取按键状态 */
#define KEY0_PRESS  (!GPIO_ReadInputDataBit(KEY0_PORT, KEY0_PIN))
#define KEY1_PRESS  (!GPIO_ReadInputDataBit(KEY1_PORT, KEY1_PIN))
#define WKUP_PRESS  (GPIO_ReadInputDataBit(WKUP_PORT, WKUP_PIN))

/* 按键返回值 */
#define KEY_NONE    0
#define KEY0_VAL    1
#define KEY1_VAL    2
#define WKUP_VAL    3

void KEY_Init(void);
uint8_t KEY_Scan(uint8_t mode);  // mode=0: 不支持连按; mode=1: 支持连按

#endif
