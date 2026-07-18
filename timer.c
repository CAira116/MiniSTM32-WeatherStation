#include "gpio.h"
#include "timer.h"


void timer_pwm_init(uint32_t freq, uint8_t duty){
// 开 GPIOA 和 TIM2 时钟
  RCC->APB1ENR |= 1 << 0;   // TIM2

 // 配 PA1 为 AF_PP
  gpio_init(GPIOA,1,AF_PP);

  // 配时基 PSC + ARR（从 freq 反算）
  // freq=72MHZ/((PSC+1)*(ARR+1))
TIM2->PSC=72-1;
  // 配 CCMR1：CH2 = PWM Mode 1sasssssssss
  // CCMR1 bit15~12写110
TIM2->CCMR1 &=~(0x7<<12);
	TIM2->CCMR1 |=(0x6<<12);
	TIM2->CCMR1 |= (1 << 11);  // OC2PE 预装载使能
  // 开 CCER：CH2 输出使能
  // 对TIM2->CCER bit4写1使能，对bit5写0保持低有效
TIM2->CCER |=(1<<4);
  // 写 CCR2 初始占空比
  // TIM2->CCR2（不超过ARR随意）
uint32_t arr_val = (1000000 / freq) - 1;
  TIM2->ARR = arr_val;
	TIM2->CCR2 = duty * (arr_val + 1) / 100;
  // 开 TIM2 计数器 TIM2->CR1 |= 1 << 0
	TIM2->CR1 |= 1 << 0;
}

void timer_pwm_set_duty(uint8_t duty) {
   TIM2->CCR2 = duty * (TIM2->ARR + 1) / 100;
  }