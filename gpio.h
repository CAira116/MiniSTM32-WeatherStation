#ifndef __GPIO_H
#define __GPIO_H

#include "stm32f10x.h"

// GPIO mode macros
#define IN_FLOAT    0x04   // floating input
#define IN_PU       0x08   // pull-up input
#define OUT_PP      0x03   // push-pull output 50MHz
#define OUT_OD      0x07   // open-drain output 50MHz
#define AF_PP       0x0B   // alternate push-pull 50MHz
#define AF_OD       0x0F

// function declarations
void    gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode);     // 配引脚模式
void    gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value);   // 输出：1=高 0=低
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);                   // 读引脚：返回 1=高 0=低
void    gpio_toggle(GPIO_TypeDef *port, uint8_t pin);                 // 翻转输出

#endif
