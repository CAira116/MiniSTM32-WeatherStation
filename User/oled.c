/**
 * OLED SSD1306 驱动 (I2C 接口, 128x64 像素)
 *
 * ─────────────────────────────────────────────
 *  半导体知识链接：
 *  OLED 每个像素是一个有机发光二极管 (Organic LED)
 *  SSD1306 内部有 GDDRAM (显存) 128x64 bits
 *  行列驱动器本质是 MOSFET 开关阵列
 *  这就是模拟IC设计中 "显示驱动芯片" 的实际产品
 * ─────────────────────────────────────────────
 *
 * 与 24C02 共用 I2C1 总线 (PB6/PB7)
 * 设备地址不同，可以共存
 */
#include "oled.h"
#include "delay.h"
#include "oledfont.h"

/* I2C1 已经由 AT24C02_Init() 初始化 */
/* OLED 复用同一 I2C1 */

/* 显存: 128 列 x 8 页 = 1024 字节 */
/* 每页 8 行像素, 共 64 行 */
static uint8_t OLED_GRAM[128][8];

/**
 * 写命令到 OLED
 */
static void OLED_WriteCmd(uint8_t cmd)
{
    // I2C1 应该在之前已初始化 (AT24C02_Init 或 OLED_Init)
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, OLED_I2C_ADDR, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    // 控制字节: 0x00 = 下一个字节是命令
    I2C_SendData(I2C1, 0x00);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(I2C1, cmd);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

/**
 * 写数据到 OLED
 */
static void OLED_WriteData(uint8_t dat)
{
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, OLED_I2C_ADDR, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    // 控制字节: 0x40 = 下一个字节是数据
    I2C_SendData(I2C1, 0x40);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(I2C1, dat);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

/**
 * SSD1306 初始化序列
 */
void OLED_Init(void)
{
    // OLED 和 24C02 共用 I2C1, 由 AT24C02_Init 或此处初始化
    // 如果之前没初始化，这里需要初始化 GPIO 和 I2C
    // 实际上建议在主函数里统一调用 AT24C02_Init (包含 I2C 初始化)

    Delay_ms(100);  // 等待 OLED 上电稳定

    OLED_WriteCmd(0xAE);  // Display OFF

    OLED_WriteCmd(0xD5);  // 设置显示时钟分频
    OLED_WriteCmd(0x80);  // 默认值

    OLED_WriteCmd(0xA8);  // 设置多路复用比
    OLED_WriteCmd(0x3F);  // 64

    OLED_WriteCmd(0xD3);  // 设置显示偏移
    OLED_WriteCmd(0x00);

    OLED_WriteCmd(0x40);  // 设置起始行

    OLED_WriteCmd(0x8D);  // 电荷泵
    OLED_WriteCmd(0x14);  // 使能

    OLED_WriteCmd(0x20);  // 内存地址模式
    OLED_WriteCmd(0x00);  // 水平模式

    OLED_WriteCmd(0xA1);  // 段重映射: 列127=SEG0
    OLED_WriteCmd(0xC8);  // COM 扫描方向: COM63→COM0

    OLED_WriteCmd(0xDA);  // COM 硬件配置
    OLED_WriteCmd(0x12);

    OLED_WriteCmd(0x81);  // 对比度
    OLED_WriteCmd(0xCF);

    OLED_WriteCmd(0xD9);  // 预充电周期
    OLED_WriteCmd(0xF1);

    OLED_WriteCmd(0xDB);  // VCOMH 电压
    OLED_WriteCmd(0x40);

    OLED_WriteCmd(0xA4);  // 全局显示: 跟随 RAM
    OLED_WriteCmd(0xA6);  // 正常显示 (非反色)

    OLED_WriteCmd(0x2E);  // 停止滚动

    OLED_Clear(0);        // 清显存
    OLED_Refresh();

    OLED_WriteCmd(0xAF);  // Display ON
}

/**
 * 清屏
 */
void OLED_Clear(uint8_t color)
{
    uint8_t fill = color ? 0xFF : 0x00;
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 128; x++) {
            OLED_GRAM[x][y] = fill;
        }
    }
}

/**
 * 将显存全量刷新到 OLED
 */
void OLED_Refresh(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        // 设置页地址
        OLED_WriteCmd(0xB0 + page);
        // 设置列低地址
        OLED_WriteCmd(0x00);
        // 设置列高地址
        OLED_WriteCmd(0x10);

        // 写一页数据 (128 字节)
        for (uint8_t col = 0; col < 128; col++) {
            OLED_WriteData(OLED_GRAM[col][page]);
        }
    }
}

/**
 * 画点
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    if (color) {
        OLED_GRAM[x][y / 8] |=  (1 << (y % 8));
    } else {
        OLED_GRAM[x][y / 8] &= ~(1 << (y % 8));
    }
}

/**
 * 显示一个 6x12 或 8x16 字符
 * size=12: 6x12 字体 (适合中文 OLED)
 * size=16: 8x16 字体
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    uint8_t i, j;
    uint8_t temp;

    ch = ch - ' ';  // 偏移到字库起始位置(空格)

    if (size == 12) {
        // 6x12 字体: 每字符 12 字节
        for (i = 0; i < 12; i++) {
            if (i < 6) {
                // 上半部分 6 列
                temp = oled_6x12[ch][i];
                for (j = 0; j < 8; j++) {
                    if (temp & (1 << j)) OLED_DrawPoint(x + i, y + j, 1);
                    else                  OLED_DrawPoint(x + i, y + j, 0);
                }
            } else {
                // 下半部分 6 列
                temp = oled_6x12[ch][i - 6];  // 复用同样的列模式，显示在第2页
                for (j = 0; j < 4; j++) {     // 只用低4位
                    if (temp & (1 << j)) OLED_DrawPoint(x + i - 6, y + 8 + j, 1);
                    else                  OLED_DrawPoint(x + i - 6, y + 8 + j, 0);
                }
            }
        }
    } else if (size == 16) {
        // 8x16 字体: 每字符 16 字节
        // 上半部分 8 字节
        for (i = 0; i < 8; i++) {
            temp = oled_8x16[ch][i];
            for (j = 0; j < 8; j++) {
                if (temp & (1 << j)) OLED_DrawPoint(x + i, y + j, 1);
                else                 OLED_DrawPoint(x + i, y + j, 0);
            }
        }
        // 下半部分 8 字节
        for (i = 0; i < 8; i++) {
            temp = oled_8x16[ch][i + 8];
            for (j = 0; j < 8; j++) {
                if (temp & (1 << j)) OLED_DrawPoint(x + i, y + 8 + j, 1);
                else                 OLED_DrawPoint(x + i, y + 8 + j, 0);
            }
        }
    }
}

/**
 * 显示字符串
 */
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    while (*str) {
        OLED_ShowChar(x, y, *str, size);
        x += (size == 12) ? 6 : 8;
        if (x + ((size == 12) ? 6 : 8) > OLED_WIDTH) {
            x = 0;
            y += 2;  // 换行(每字符占2页)
        }
        str++;
    }
}

/**
 * 显示数字
 */
void OLED_ShowNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size)
{
    char buf[12];
    uint8_t i;

    if (num < 0) {
        OLED_ShowChar(x, y, '-', size);
        num = -num;
        x += (size == 12) ? 6 : 8;
    }

    // 转换为字符串
    i = 0;
    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num) {
            buf[i++] = num % 10 + '0';
            num /= 10;
        }
    }

    // 补空格到指定长度
    while (i < len) buf[i++] = ' ';

    // 从后往前显示 (因为 buf 是反向的)
    for (uint8_t j = i; j > 0; j--) {
        OLED_ShowChar(x, y, buf[j - 1], size);
        x += (size == 12) ? 6 : 8;
    }
}

/**
 * 显示浮点数
 */
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t intLen, uint8_t decLen, uint8_t size)
{
    int32_t intPart, decPart;

    if (num < 0) {
        OLED_ShowChar(x, y, '-', size);
        num = -num;
        x += (size == 12) ? 6 : 8;
    }

    intPart = (int32_t)num;
    decPart = (int32_t)((num - intPart) * 10 + 0.5f);  // 1位小数

    OLED_ShowNum(x, y, intPart, intLen, size);
    x += intLen * ((size == 12) ? 6 : 8);

    OLED_ShowChar(x, y, '.', size);
    x += (size == 12) ? 6 : 8;

    OLED_ShowNum(x, y, decPart, decLen, size);
}

/**
 * 开机画面
 */
void OLED_ShowBootScreen(void)
{
    OLED_Clear(0);

    OLED_ShowString(0, 0, "MiniSTM32", 16);
    OLED_ShowString(0, 2, "Weather Station", 16);

    OLED_ShowString(0, 4, "Temp: --.- C", 12);
    OLED_ShowString(0, 6, "Initializing...", 12);

    OLED_Refresh();
    Delay_ms(1000);
}
