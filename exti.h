#ifndef __EXTI_H
#define __EXTI_H
#define EXTI_FALLING  0   // �½��أ����°�����
#define EXTI_RISING   1   // �����أ��ɿ�������

#include "stm32f10x.h"

  void exti_init(GPIO_TypeDef *port, uint8_t pin, uint8_t trigger);

  extern volatile uint8_t button_flag;

#endif
