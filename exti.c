#include "exti.h"
#include "systick.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t hDisplayTask;

void EXTI1_IRQHandler(void){
 BaseType_t woken = pdFALSE;
	
	if(gpio_read(GPIOA,1)==0){
	vTaskNotifyGiveFromISR(hDisplayTask,&woken);
	}
	EXTI->PR |=(1<<1);
	portYIELD_FROM_ISR(woken);
}

/*
 * EXTI 初始化
 * port  : 哪个GPIO口（GPIOA/B/C）
 * pin   : 第几号引脚（0~15）
 * trigger: 什么电平变化触发中断（EXTI_FALLING=按下 / EXTI_RISING=松开）
 *
 * 大白话流程（六步走）：
 * ① 开AFIO时钟  → ② 把GPIO口接到EXTI线上（AFIO接线板）
 * ③ 允许这条线触发中断（IMR）  → ④ 算IRQ编号，打开NVIC总闸（ISER）
 * ⑤ 选触发方式（下降沿还是上升沿）  → ⑥ 把引脚配成上拉输入
 *
 * ⚠️ 已知坑：irq_channel=6+pin 只对 pin 0~4 成立
 *    pin 5~9 合租 IRQ23，pin 10~15 合租 IRQ40，公式不适用
 */
void exti_init(GPIO_TypeDef *port, uint8_t pin, uint8_t trigger){
    // ① 开AFIO时钟（AFIO在APB2总线上，bit0=电源闸刀，不开的话后面EXTICR写了白写）
    RCC->APB2ENR |= (1<<0);

    // ② AFIO接线板：把GPIO口接到EXTI线上
    uint8_t port_idx;                        // AFIO只认数字编号
    if (port == GPIOA)       port_idx = 0;   // GPIOA → 编号0
    else if (port == GPIOB)  port_idx = 1;   // GPIOB → 编号1
    else if (port == GPIOC)  port_idx = 2;   // GPIOC → 编号2
    // pin/4 → 选EXTICR[0]~[3]  ｜  pin%4 → 该寄存器里的第几号坑位（每坑占4bit）
    AFIO->EXTICR[pin / 4] |= (port_idx << ((pin % 4) * 4));

    // ③ IMR中断开关：允许这条EXTI线触发中断（32位寄存器，1位=1条线）
    EXTI->IMR |= (1<<pin);

    // ④ NVIC总闸：允许这个IRQ编号的中断到达CPU
    uint8_t irq_channel = 6 + pin;           // EXTI0→IRQ6, EXTI1→IRQ7...（仅pin0~4正确！）
    // irq_channel/32 → ISER[0]还是ISER[1]  ｜  irq_channel%32 → 寄存器里第几位
    NVIC->ISER[irq_channel/32] |= (1 << (irq_channel % 32));

    // ⑤ 选触发方式
    if (trigger == EXTI_FALLING){
        EXTI->FTSR |= (1<<pin);              // 下降沿检测：高→低（按键按下）
    }
    else if (trigger == EXTI_RISING){
        EXTI->RTSR |= (1<<pin);              // 上升沿检测：低→高（按键松开）
    }

    // ⑥ 引脚模式：上拉输入（不按时电阻拉到高电平，按下拉到低电平→产生下降沿）
    gpio_init(port, pin, IN_PU);
}

