#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f10x.h"
#include <stdio.h>

/* 系统时钟 */
#define SYSTEM_CLOCK    72000000  // 72MHz

/* EEPROM 地址定义 */
#define EE_ADDR_MAX_TEMP    0x00  // 最高温度记录 (float, 4字节)
#define EE_ADDR_MIN_TEMP    0x04  // 最低温度记录 (float, 4字节)

/* 气象站显示模式 */
#define MODE_CURRENT     0  // 当前温度
#define MODE_MAX_MIN     1  // 最高/最低记录
#define MODE_HISTORY     2  // 温度曲线 (简易)

/* 全局变量 */
extern uint8_t  g_display_mode;   // 当前显示模式
extern float    g_temp_current;   // 当前温度
extern float    g_temp_max;       // 历史最高温
extern float    g_temp_min;       // 历史最低温
extern uint32_t g_tick;           // 系统心跳计数 (1秒递增)

/* 系统初始化 */
void SystemClock_Config(void);    // 配置系统时钟 72MHz
void USART1_Init(uint32_t baud);  // 初始化串口
void ADC1_Init(void);             // 初始化 ADC (内部温度传感器)
float GetMCUTemperature(void);    // 读取 MCU 内部温度

#endif
