#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

/* OLED SSD1306 I2C 地址: 0x3C (7-bit) → 写 0x78, 读 0x79 */
#define OLED_I2C_ADDR   0x78   // 写地址

/* OLED 分辨率 */
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

/**
 * 初始化 OLED
 * 必须在使用其他函数前调用
 */
void OLED_Init(void);

/**
 * 清屏
 * @param color: 0=全黑, 1=全亮
 */
void OLED_Clear(uint8_t color);

/**
 * 在指定位置显示一个 6x8 或 8x16 的 ASCII 字符
 * @param x: 列 (0-127)
 * @param y: 页 (0-7, 每页8像素高)
 * @param ch: ASCII 字符
 * @param size: 字号 12=6x12, 16=8x16
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size);

/**
 * 显示字符串
 */
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);

/**
 * 显示有符号整数
 */
void OLED_ShowNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size);

/**
 * 显示浮点数 (保留1位小数)
 */
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t intLen, uint8_t decLen, uint8_t size);

/**
 * 画点 (直接操作显存)
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color);

/**
 * 将显存刷新到 OLED (必须调用才会显示)
 */
void OLED_Refresh(void);

/**
 * 显示开机画面
 */
void OLED_ShowBootScreen(void);

#endif
