/**
 * GPIO 全扫描 - 找到 LED 的准确引脚
 *
 * 依次点亮 PA0~PA15, PB0~PB15, PC13~PC15 中的每一个
 * 每 0.5 秒切换一个引脚，你能看到 LED 在哪一步亮起
 *
 * 串口同步输出当前测试的引脚号
 */
#include "stm32f10x.h"
#include <stdio.h>

static uint32_t fac_us = 0;

void Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;
    SystemCoreClockUpdate();
    fac_us = SystemCoreClock / 1000000;
}

void Delay_ms(uint32_t nms)
{
    for (uint32_t i = 0; i < nms; i++) {
        uint32_t start = DWT->CYCCNT;
        uint32_t ticks = fac_us * 1000;
        while ((DWT->CYCCNT - start) < ticks);
    }
}

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

static void USART_Init_Minimal(uint32_t baud)
{
    GPIO_InitTypeDef  gpio;
    USART_InitTypeDef usart;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_9; gpio.GPIO_Mode = GPIO_Mode_AF_PP; gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10; gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
    USART_StructInit(&usart); usart.USART_BaudRate = baud; usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart); USART_Cmd(USART1, ENABLE);
}

int fputc(int ch, FILE *f) {
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

typedef struct { GPIO_TypeDef *port; uint16_t pin; char name[8]; } PinDef;

int main(void)
{
    Clock_HSI();
    Delay_Init();
    USART_Init_Minimal(115200);

    printf("\r\n===================================\r\n");
    printf("  GPIO Pin Scanner\r\n");
    printf("  Look for LED blink!\r\n");
    printf("===================================\r\n");

    /* 测试列表: 所有常见 LED 引脚 */
    PinDef pins[] = {
        {GPIOA, GPIO_Pin_5,  "PA5"},    /* 原理图标注 */
        {GPIOC, GPIO_Pin_13, "PC13"},   /* Blue Pill 常见 */
        {GPIOA, GPIO_Pin_0,  "PA0"},
        {GPIOA, GPIO_Pin_1,  "PA1"},
        {GPIOA, GPIO_Pin_2,  "PA2"},
        {GPIOA, GPIO_Pin_3,  "PA3"},
        {GPIOA, GPIO_Pin_4,  "PA4"},
        {GPIOA, GPIO_Pin_6,  "PA6"},
        {GPIOA, GPIO_Pin_7,  "PA7"},
        {GPIOB, GPIO_Pin_0,  "PB0"},
        {GPIOB, GPIO_Pin_1,  "PB1"},
        {GPIOB, GPIO_Pin_5,  "PB5"},
        {GPIOB, GPIO_Pin_6,  "PB6"},
        {GPIOB, GPIO_Pin_7,  "PB7"},
        {GPIOB, GPIO_Pin_8,  "PB8"},
        {GPIOC, GPIO_Pin_14, "PC14"},
        {GPIOC, GPIO_Pin_15, "PC15"},
    };
    int nPins = sizeof(pins) / sizeof(pins[0]);

    printf("Total %d pins to test, 0.5s each\r\n", nPins);

    /* 初始化所有端口 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    printf("GPIO clocks enabled\r\n");

    GPIO_InitTypeDef g;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;

    /* 先全部初始化为高电平 */
    for (int i = 0; i < nPins; i++) {
        g.GPIO_Pin = pins[i].pin;
        GPIO_Init(pins[i].port, &g);
        pins[i].port->BSRR = pins[i].pin;  // HIGH = OFF
    }
    printf("All pins set HIGH (OFF)\r\n");

    while (1) {
        for (int i = 0; i < nPins; i++) {
            printf("[%d/%d] Testing %s... ", i+1, nPins, pins[i].name);

            /* 闪 3 次 */
            for (int j = 0; j < 3; j++) {
                pins[i].port->BRR = pins[i].pin;   // ON
                Delay_ms(200);
                pins[i].port->BSRR = pins[i].pin;  // OFF
                Delay_ms(200);
            }

            printf("done\r\n");
        }
        printf("--- Round complete, restarting ---\r\n");
    }
}
