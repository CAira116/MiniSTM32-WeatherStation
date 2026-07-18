#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f10x.h"

 // freq: PWM频率(Hz), duty: 占空比0-100
void timer_pwm_init(uint32_t freq,uint8_t duty);
 // duty: 占空比0-100
void timer_pwm_set_duty(uint8_t duty);

#endif