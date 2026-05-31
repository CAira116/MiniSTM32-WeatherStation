/**
 * BRUTE FORCE BLINK
 * No clock config, no UART, no delay - just raw GPIO toggle
 * Uses startup default clock (HSI 8MHz after reset)
 */
#include "stm32f10x.h"

void dly(void) {
    volatile unsigned long i;
    for(i=0; i<500000; i++);
}

int main(void)
{
    GPIO_InitTypeDef g;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;

    /* Init ALL ports */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC, ENABLE);

    /* All pins as output */
    g.GPIO_Pin = 0xFFFF;
    GPIO_Init(GPIOA, &g);
    GPIO_Init(GPIOB, &g);
    GPIO_Init(GPIOC, &g);

    /* ALL HIGH */
    GPIOA->BSRR = 0xFFFF;
    GPIOB->BSRR = 0xFFFF;
    GPIOC->BSRR = 0xFFFF;

    while(1) {
        /* ALL LOW */
        GPIOA->BRR = 0xFFFF;
        GPIOB->BRR = 0xFFFF;
        GPIOC->BRR = 0xFFFF;
        dly();

        /* ALL HIGH */
        GPIOA->BSRR = 0xFFFF;
        GPIOB->BSRR = 0xFFFF;
        GPIOC->BSRR = 0xFFFF;
        dly();
    }
}
