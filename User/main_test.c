/**
 * MiniSTM32 V2 最简测试 - LED闪烁 + 串口输出
 *
 * 根据原理图确认的硬件:
 *   LED:  PA5, 低电平点亮, R17=220R 限流
 *   按键: PA0, 100K上拉, 按下接地
 *   串口: USART1 (PA9/PA10) → CH340G
 *   BOOT0: 10K下拉 + 跳线接口
 */
#include "stm32f10x.h"
#include <stdio.h>

/* ---- 微秒延时 (DWT) ---- */
static uint32_t fac_us = 0;

void Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;
    SystemCoreClockUpdate();
    fac_us = SystemCoreClock / 1000000;
}

void Delay_us(uint32_t nus)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = nus * fac_us;
    while ((DWT->CYCCNT - start) < ticks);
}

void Delay_ms(uint32_t nms)
{
    for (uint32_t i = 0; i < nms; i++) Delay_us(1000);
}

/* ---- LED 初始化: PA5, 推挽输出 ---- */
static void LED_Init(void)
{
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    g.GPIO_Pin   = GPIO_Pin_5;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    GPIOA->BSRR = GPIO_Pin_5;  // PA5=HIGH → LED 灭
}

/* ---- 按键初始化: PA0, 上拉输入 ---- */
static void KEY_Init(void)
{
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    g.GPIO_Pin  = GPIO_Pin_0;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &g);
}

/* ---- USART1 初始化 ---- */
static void USART_Init_Minimal(uint32_t baud)
{
    GPIO_InitTypeDef  gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_9;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin  = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_StructInit(&usart);
    usart.USART_BaudRate = baud;
    usart.USART_Mode     = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

/* ---- 时钟：HSI 8MHz (不依赖外部晶振) ---- */
static void Clock_HSI(void)
{
    RCC_DeInit();
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    SystemCoreClockUpdate();
}

/* ---- 主函数 ---- */
int main(void)
{
    uint32_t tick = 0;
    uint8_t  key_pressed = 0;

    Clock_HSI();
    Delay_Init();
    LED_Init();
    KEY_Init();
    USART_Init_Minimal(115200);

    /* 启动信号: LED 快闪 3 次 */
    for (int i = 0; i < 3; i++) {
        GPIOA->BRR = GPIO_Pin_5;   // PA5 LOW  = LED ON
        Delay_ms(150);
        GPIOA->BSRR = GPIO_Pin_5;  // PA5 HIGH = LED OFF
        Delay_ms(150);
    }

    printf("\r\n===================================\r\n");
    printf("  MiniSTM32 V2 Blink Test\r\n");
    printf("  LED: PA5 (active LOW)\r\n");
    printf("  KEY: PA0 (press = GND)\r\n");
    printf("  MCU: STM32F103 @ 8MHz HSI\r\n");
    printf("  Press KEY(SH1) to toggle speed\r\n");
    printf("===================================\r\n");

    while (1) {
        tick++;

        /* 检测按键: PA0 = 0 表示按下 */
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) {
            Delay_ms(20);  // 消抖
            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) {
                if (!key_pressed) {
                    key_pressed = 1;
                    printf("KEY pressed! tick=%lu\r\n", tick);
                }
            }
        } else {
            key_pressed = 0;
        }

        /* LED 闪烁: 1秒亮/1秒灭 */
        if (tick & 1) {
            GPIOA->BRR = GPIO_Pin_5;   // LED ON
        } else {
            GPIOA->BSRR = GPIO_Pin_5;  // LED OFF
        }

        printf("Tick=%lu LED=%s\r\n", tick, (tick & 1) ? "ON" : "OFF");

        Delay_ms(1000);
    }
}
