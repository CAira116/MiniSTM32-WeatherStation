/**
 * LED 驱动
 * MiniSTM32 V3.0: 低电平点亮
 *   LED0 = PE5 (DS0, 红色)
 *   LED1 = PB5 (DS1, 绿色)
 */
#include "led.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 开启 GPIO 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOB, ENABLE);

    // PE5 → LED0 (推挽输出, 50MHz)
    GPIO_InitStructure.GPIO_Pin   = LED0_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED0_PORT, &GPIO_InitStructure);

    // PB5 → LED1
    GPIO_InitStructure.GPIO_Pin   = LED1_PIN;
    GPIO_Init(LED1_PORT, &GPIO_InitStructure);

    // 初始状态：全灭
    LED0_OFF();
    LED1_OFF();
}
