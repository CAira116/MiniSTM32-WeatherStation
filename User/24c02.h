#ifndef __24C02_H
#define __24C02_H

#include "stm32f10x.h"

/* 24C02 EEPROM: I2C1, 与 OLED 共用总线 */
/* 设备地址: 1010 A2 A1 A0 R/W → 1010000 = 0x50 (7-bit) */
#define AT24C02_ADDR_WRITE  0xA0  // 写地址
#define AT24C02_ADDR_READ   0xA1  // 读地址
#define AT24C02_SIZE        256   // 256 字节

void AT24C02_Init(void);
void AT24C02_WriteByte(uint8_t addr, uint8_t dat);
uint8_t AT24C02_ReadByte(uint8_t addr);
void AT24C02_WriteBytes(uint8_t addr, uint8_t *buf, uint8_t len);
void AT24C02_ReadBytes(uint8_t addr, uint8_t *buf, uint8_t len);

#endif
