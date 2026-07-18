#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "stm32f10x.h"

void systick_delay_ms(uint32_t ms);
void systick_delay_us(uint32_t us);
void systick_clock_init(void);
uint32_t systick_get_ms(void);

#endif
