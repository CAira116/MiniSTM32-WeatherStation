#include "systick.h"

volatile uint32_t ms_ticks;

void SysTick_Handler(void){
ms_ticks++;//每过1ms，变量加1
}

void systick_clock_init(void){

	 SysTick->CTRL &= ~(1 << 2);
	 SysTick->LOAD = 9000-1;  //配置LOAD
	 SysTick->VAL=0;//VAL清零
	 SysTick->CTRL |= (1<<1);//开中断
	    SysTick->CTRL |= 1;//开定时器
 }

 uint32_t systick_get_ms(void)   {
	return ms_ticks;//返回当前时间
	}
	
 
//前提：必须先 systick_clock_init()，否则 get_ms 永远报 0，while永远为真，死循环
	void systick_delay_ms(uint32_t ms) {
     uint32_t b;
      b=systick_get_ms();// ① 出发时间：看一眼表，存进一个 uint32_t 变量
      while((systick_get_ms()-b)<ms){
      }// ② 原地转圈：只要（现在 - 出发时间）< ms，就继续看
  }

// 微秒延时（最大 1.86ms）
// 注意：借表期间时钟暂停，每次调用表慢 us微秒
void systick_delay_us(uint32_t us) {
	  SysTick->CTRL &= ~(1 << 1);
    SysTick->CTRL &= ~(1 << 2);  // HCLK/8 = 9MHz
    SysTick->LOAD = 9 * us - 1;   // 1us = 9 ticks
    SysTick->VAL = 0;
    SysTick->CTRL |= 1;
    while (!(SysTick->CTRL & (1 << 16)));
	  systick_clock_init();
}

 
 
  
	