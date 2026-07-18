#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"

void    usart_init(USART_TypeDef *usart, uint32_t baud);
void    usart2_init(uint32_t baud);
void    usart_send_byte(USART_TypeDef *usart, uint8_t data);
void    usart_send_string(USART_TypeDef *usart, const char *str);
uint8_t usart_receive_byte(USART_TypeDef *usart);
int     usart_try_receive(USART_TypeDef *usart, uint8_t*out);
#endif


