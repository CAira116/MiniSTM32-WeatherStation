/**
 * Final LED Test - PA5, BOTH polarities
 * Phase 1: Active LOW (PA5=0 = LED ON)  - 5 blinks
 * Phase 2: Active HIGH (PA5=1 = LED ON) - 5 blinks
 *
 * Whichever phase makes the LED blink is the correct polarity.
 * Serial output: 115200 baud
 */
#include "stm32f10x.h"
#include <stdio.h>

static uint32_t fac_us;
void Delay_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;
    SystemCoreClockUpdate();
    fac_us = SystemCoreClock / 1000000;
}
void Delay_ms(uint32_t n) {
    for(uint32_t i=0;i<n;i++) {
        uint32_t s=DWT->CYCCNT, t=fac_us*1000;
        while((DWT->CYCCNT-s)<t);
    }
}
int fputc(int ch, FILE *f) {
    USART_SendData(USART1, (uint8_t)ch);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET);
    return ch;
}

int main(void) {
    GPIO_InitTypeDef g;
    USART_InitTypeDef u;

    /* Clock: HSI 8MHz */
    RCC_DeInit();
    RCC_HSICmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY)==RESET);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    SystemCoreClockUpdate();
    Delay_Init();

    /* GPIOA clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA5 push-pull output */
    g.GPIO_Pin = GPIO_Pin_5;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    /* USART1 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    g.GPIO_Pin = GPIO_Pin_9; g.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &g);
    g.GPIO_Pin = GPIO_Pin_10; g.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &g);
    USART_StructInit(&u);
    u.USART_BaudRate = 115200;
    u.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &u);
    USART_Cmd(USART1, ENABLE);

    printf("\r\n===================================\r\n");
    printf("  LED Polarity Test - PA5\r\n");
    printf("  CHIP: STM32F103C8T6\r\n");
    printf("  CLOCK: %lu Hz\r\n", SystemCoreClock);
    printf("===================================\r\n");

    while (1) {
        /* Phase 1: Active LOW test */
        printf("--- Phase 1: Active LOW (PA5=0 => LED ON) ---\r\n");
        GPIOA->BSRR = GPIO_Pin_5; // HIGH first
        Delay_ms(500);
        for (int i = 0; i < 5; i++) {
            GPIOA->BRR = GPIO_Pin_5;  // LOW = should turn ON if active low
            printf("  LOW (LED should be ON)\r\n");
            Delay_ms(500);
            GPIOA->BSRR = GPIO_Pin_5; // HIGH = should turn OFF
            printf("  HIGH (LED should be OFF)\r\n");
            Delay_ms(500);
        }

        /* Phase 2: Active HIGH test */
        printf("--- Phase 2: Active HIGH (PA5=1 => LED ON) ---\r\n");
        GPIOA->BRR = GPIO_Pin_5; // LOW first
        Delay_ms(500);
        for (int i = 0; i < 5; i++) {
            GPIOA->BSRR = GPIO_Pin_5; // HIGH = should turn ON if active high
            printf("  HIGH (LED should be ON)\r\n");
            Delay_ms(500);
            GPIOA->BRR = GPIO_Pin_5;  // LOW = should turn OFF
            printf("  LOW (LED should be OFF)\r\n");
            Delay_ms(500);
        }

        printf("--- Cycle complete. If LED never blinked, try other chip ---\r\n");
        Delay_ms(2000);
    }
}
