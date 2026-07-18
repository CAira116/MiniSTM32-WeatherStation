#ifndef __SSD1306_H
#define __SSD1306_H
#include "stm32f10x.h"

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_show_str(uint8_t line, const char *str);

#endif
