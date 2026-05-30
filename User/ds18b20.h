#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f10x.h"

/* DS18B20 引脚: PG11 */
#define DS18B20_PORT    GPIOG
#define DS18B20_PIN     GPIO_Pin_11
#define DS18B20_RCC     RCC_APB2Periph_GPIOG

/* DS18B20 单总线操作宏
 *
 * 关键技巧：操作 GPIO 方向来模拟开漏输出
 *   输出模式 + 推挽 = 拉低总线
 *   输入模式 + 上拉 = 释放总线(外部上拉电阻拉高)
 *
 * 这利用了 STM32 GPIO 的端口配置寄存器(CRL/CRH)实现快速切换
 * —— 比调用库函数快得多，满足单总线微秒级时序要求
 */

// 总线拉低: 设为推挽输出, 输出0
#define DS18B20_OUT_LO()   do { \
    DS18B20_PORT->CRH &= 0xFFFF0FFF; \
    DS18B20_PORT->CRH |= 0x00003000; \
    DS18B20_PORT->BRR = DS18B20_PIN; \
} while(0)

// 释放总线: 设为上拉输入 (外部 4.7kΩ 电阻拉高)
#define DS18B20_IN_FLOAT() do { \
    DS18B20_PORT->CRH &= 0xFFFF0FFF; \
    DS18B20_PORT->CRH |= 0x00008000; \
} while(0)

// 读取总线电平
#define DS18B20_READ()     (GPIO_ReadInputDataBit(DS18B20_PORT, DS18B20_PIN))

// 总线拉高（用于强驱动场景）
#define DS18B20_OUT_HI()   do { \
    DS18B20_PORT->CRH &= 0xFFFF0FFF; \
    DS18B20_PORT->CRH |= 0x00003000; \
    DS18B20_PORT->BSRR = DS18B20_PIN; \
} while(0)

/* DS18B20 ROM 命令 */
#define DS18B20_CMD_SEARCH_ROM     0xF0
#define DS18B20_CMD_READ_ROM       0x33
#define DS18B20_CMD_MATCH_ROM      0x55
#define DS18B20_CMD_SKIP_ROM       0xCC  // 跳过 ROM(单设备时使用)
#define DS18B20_CMD_ALARM_SEARCH   0xEC

/* DS18B20 功能命令 */
#define DS18B20_CMD_CONVERT_T      0x44  // 启动温度转换
#define DS18B20_CMD_WRITE_SCRATCH  0x4E  // 写暂存器
#define DS18B20_CMD_READ_SCRATCH   0xBE  // 读暂存器(9字节)
#define DS18B20_CMD_COPY_SCRATCH   0x48  // 复制暂存器到 EEPROM
#define DS18B20_CMD_RECALL_EE      0xB8  // 从 EEPROM 召回
#define DS18B20_CMD_READ_POWER     0xB4  // 读供电模式

uint8_t  DS18B20_Init(void);           // 初始化，返回 0=成功
void     DS18B20_WriteByte(uint8_t dat); // 写一个字节
uint8_t  DS18B20_ReadByte(void);         // 读一个字节
float    DS18B20_GetTemperature(void);   // 获取温度(°C)

#endif
