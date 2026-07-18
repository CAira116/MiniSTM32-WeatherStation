#include "usart.h"
#include "gpio.h"
#include <stdio.h>

void usart_init(USART_TypeDef *usart, uint32_t baud)
{
    RCC->APB2ENR |= 1 << 2;       // GPIOA clock
    RCC->APB2ENR |= 1 << 14;      // USART1 clock

    gpio_init(GPIOA, 9, AF_PP);   // TX = alternate push-pull
    gpio_init(GPIOA, 10, IN_FLOAT); // RX = floating input

    usart->BRR = 72000000 / baud; // baud rate

    usart->CR1 |= (1 << 3);       // TE: transmitter enable
    usart->CR1 |= (1 << 2);       // RE: receiver enable
    usart->CR1 |= (1 << 13);      // UE: USART enable
}
 void usart2_init(uint32_t baud)
  {
      RCC->APB2ENR |= 1 << 2;      // GPIOA 时钟，不变
      RCC->APB1ENR |= 1 << 17;    // ① USART2EN 第几位？查手册 RCC_APB1ENR 那页
      gpio_init(GPIOA, 2, AF_PP);      // ② TX 是 PA几？
      gpio_init(GPIOA, 3, IN_FLOAT);   // ③ RX 是 PA几？
      USART2->BRR = 36000000 / baud;
      USART2->CR1 |= (1 << 3);     // TE
      USART2->CR1 |= (1 << 2);     // RE
      USART2->CR1 |= (1 << 13);    // UE
  }

void usart_send_byte(USART_TypeDef *usart, uint8_t data)
{
    while (!(usart->SR & (1 << 7))); // wait TXE=1 (DR empty)
    usart->DR = data;
}

void usart_send_string(USART_TypeDef *usart, const char *str)
{
    while (*str) {
        usart_send_byte(usart, *str);
        str++;
    }
}

uint8_t usart_receive_byte(USART_TypeDef *usart)
{
    while (!(usart->SR & (1 << 5))); // wait RXNE=1 (data received)
    return (uint8_t)(usart->DR);     // read DR, RXNE auto-clears
}

/* printf redirect: override the low-level character output */
int fputc(int ch, FILE *f)
{
    usart_send_byte(USART1, (uint8_t)ch);
    return ch;
}

 int usart_try_receive(USART_TypeDef *usart, uint8_t*out){
	if ((usart->SR & (1<<5)) != 0)  {
	*out= (uint8_t)(usart->DR);
		return 1;
	}
	return 0;
	}