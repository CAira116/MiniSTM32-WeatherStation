/**
 * ╔══════════════════════════════════════════════════════════╗
 * ║     桌面气象站 - Desktop Weather Station               ║
 * ║     基于 MiniSTM32 V3.0 (STM32F103RCT6)                 ║
 * ║     开发环境: Keil MDK + 标准外设库                     ║
 * ╚══════════════════════════════════════════════════════════╝
 *
 * 功能:
 *   - DS18B20 外部温度采集 (分辨率 0.0625°C)
 *   - MCU 内部温度传感器 (ADC1 通道16)
 *   - OLED 128x64 实时显示 (I2C)
 *   - 串口数据上报 (USART1 → CH340G → PC)
 *   - 最高/最低温度记录 (AT24C02 EEPROM)
 *   - 按键切换显示模式 (KEY0/KEY1/WK_UP)
 *   - LED 心跳指示
 *
 * 硬件连接:
 *   见 docs/pinout.md
 *
 * ═══════════════════════════════════════════════════════════
 *  IC 知识扩展 (面试/考研加分项):
 *  ═══════════════════════════════════════════════════════════
 *  1. DS18B20 测量链: 温度 → bandgap 电压 → Σ-Δ ADC → 数字
 *  2. 单总线寄生供电: MOSFET + 电容储能, 体现低功耗设计思想
 *  3. I2C 开漏输出: 线与逻辑, 每个 SDA/SCL 需要上拉电阻
 *  4. EEPROM 浮栅晶体管: FN 隧穿写入, 和 Flash 的区别
 *  5. MCU 内部温度: 利用 BJT 的 V_BE 负温度系数 (~-2mV/°C)
 * ═══════════════════════════════════════════════════════════
 */

#include "main.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "ds18b20.h"
#include "oled.h"
#include "24c02.h"

/* ─── 全局变量 ─── */
uint8_t  g_display_mode = MODE_CURRENT;
float    g_temp_current = 0.0f;
float    g_temp_max     = -40.0f;   // DS18B20 最低量程
float    g_temp_min     = 125.0f;   // DS18B20 最高量程
uint32_t g_tick         = 0;

/* 温度历史记录缓冲区 (简易, 存最近128次读数) */
#define HIST_SIZE  128
static float g_temp_history[HIST_SIZE] = {0};
static uint8_t g_hist_index = 0;

/* ─── 系统时钟配置: HSE 8MHz → PLL ×9 → 72MHz ─── */
void SystemClock_Config(void)
{
    ErrorStatus HSEStartUpStatus;

    /* 1. 复位 RCC 配置 */
    RCC_DeInit();

    /* 2. 使能外部高速晶振 (HSE = 8MHz) */
    RCC_HSEConfig(RCC_HSE_ON);

    /* 3. 等待 HSE 稳定 */
    HSEStartUpStatus = RCC_WaitForHSEStartUp();
    if (HSEStartUpStatus == SUCCESS) {
        /* 4. 使能预取缓冲 */
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

        /* 5. 设置 Flash 等待周期 (72MHz → 需要2个等待周期) */
        FLASH_SetLatency(FLASH_Latency_2);

        /* 6. 设置 AHB/APB1/APB2 分频 */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);    // AHB = 72MHz
        RCC_PCLK1Config(RCC_HCLK_Div2);     // APB1 = 36MHz (最大)
        RCC_PCLK2Config(RCC_HCLK_Div1);     // APB2 = 72MHz

        /* 7. 配置 PLL: HSE × 9 = 72MHz */
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

        /* 8. 使能 PLL */
        RCC_PLLCmd(ENABLE);

        /* 9. 等待 PLL 就绪 */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

        /* 10. 切换到 PLL 作为系统时钟 */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        /* 11. 等待切换完成 */
        while (RCC_GetSYSCLKSource() != 0x08);
    }
    else {
        /* HSE 启动失败 → 死循环, LED 快速闪烁报警 */
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
        GPIO_InitTypeDef g;
        g.GPIO_Pin   = GPIO_Pin_5;
        g.GPIO_Mode  = GPIO_Mode_Out_PP;
        g.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOB, &g);
        while (1) {
            GPIOB->ODR ^= GPIO_Pin_5;
            for (volatile uint32_t i = 0; i < 500000; i++);
        }
    }
}

/* ─── 串口初始化 ─── */
void USART1_Init(uint32_t baud)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    /* PA9(TX): 复用推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA10(RX): 浮空输入 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART1 配置 */
    USART_InitStructure.USART_BaudRate            = baud;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl  = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

/* ─── 串口 printf 重定向 (在 Keil 里勾选 "Use MicroLIB") ─── */
#ifdef __MICROLIB
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}
#else
/* 如果不用 MicroLIB, 用这个: */
#pragma import(__use_no_semihosting)
struct __FILE { int handle; };
FILE __stdout;
void _sys_exit(int x) { while(1); }
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}
#endif

/* ─── ADC1 初始化 (MCU 内部温度传感器, 通道16) ─── */
void ADC1_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_DeInit(ADC1);

    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode        = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode  = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv    = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign           = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel        = 1;

    ADC_Init(ADC1, &ADC_InitStructure);

    /* 使能内部温度传感器 */
    ADC_TempSensorVrefintCmd(ENABLE);

    /* 校准 ADC */
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

/**
 * 读取 MCU 内部温度传感器
 * V_25 = 1.43V (25°C 时的电压)
 * Avg_Slope = 4.3 mV/°C (温度系数)
 * 公式: T = ((V_25 - V_sense) / Avg_Slope) + 25
 *
 * STM32F103 内部温度传感器原理:
 *   一个 PNP 晶体管的 V_BE (~0.7V) 随温度线性变化
 *   → 缓冲放大 → ADC 采样
 *   这就是半导体物理课讲的 BJT 温度特性
 *
 * 注意: 内部传感器精度约 ±1.5°C, 仅用于参考
 */
float GetMCUTemperature(void)
{
    uint16_t adc_val;
    float voltage, temperature;

    /* 选择通道 16 (内部温度传感器), 采样时间 239.5 周期 (最长, 保证精度) */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);

    /* 启动单次转换 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    /* 等待转换完成 */
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

    adc_val = ADC_GetConversionValue(ADC1);

    /* 换算为电压 (3.3V 参考, 12-bit) */
    voltage = (float)adc_val * 3.3f / 4096.0f;

    /* 换算为温度 */
    temperature = ((1.43f - voltage) / 0.0043f) + 25.0f;

    return temperature;
}

/* ─── 从 EEPROM 加载历史记录 ─── */
static void LoadRecords(void)
{
    uint8_t buf[4];

    /* 读取最高温 */
    AT24C02_ReadBytes(EE_ADDR_MAX_TEMP, buf, 4);
    float *p_max = (float *)buf;
    if (p_max && *p_max > -100.0f && *p_max < 200.0f) {
        g_temp_max = *p_max;
    }

    /* 读取最低温 */
    AT24C02_ReadBytes(EE_ADDR_MIN_TEMP, buf, 4);
    float *p_min = (float *)buf;
    if (p_min && *p_min > -100.0f && *p_min < 200.0f) {
        g_temp_min = *p_min;
    }
}

/* ─── 保存记录到 EEPROM ─── */
static void SaveRecords(void)
{
    AT24C02_WriteBytes(EE_ADDR_MAX_TEMP, (uint8_t *)&g_temp_max, 4);
    Delay_ms(10);
    AT24C02_WriteBytes(EE_ADDR_MIN_TEMP, (uint8_t *)&g_temp_min, 4);
}

/* ─── 更新温度历史 ─── */
static void UpdateHistory(float temp)
{
    g_temp_history[g_hist_index] = temp;
    g_hist_index = (g_hist_index + 1) % HIST_SIZE;
}

/* ─── 在 OLED 上画简易温度曲线 ─── */
static void DrawTrendGraph(void)
{
    /* 画坐标轴 */
    for (uint8_t x = 0; x < 128; x++) {
        OLED_DrawPoint(x, 32, 1);  // X轴
    }
    for (uint8_t y = 0; y < 64; y++) {
        OLED_DrawPoint(0, y, 1);   // Y轴
    }

    /* 绘制最近 128 个温度点 */
    /* Y 范围: 0-63 对应 0°C~50°C */
    for (uint8_t x = 0; x < HIST_SIZE && x < 128; x++) {
        /* 从最新数据开始往前画 */
        uint8_t idx = (g_hist_index - 1 - x + HIST_SIZE) % HIST_SIZE;
        if (g_temp_history[idx] == 0.0f) continue;  // 跳过未初始化数据

        float temp = g_temp_history[idx];
        if (temp < 0.0f) temp = 0.0f;
        if (temp > 50.0f) temp = 50.0f;

        uint8_t y_pos = 63 - (uint8_t)(temp * 63.0f / 50.0f);
        if (y_pos < 0) y_pos = 0;
        if (y_pos > 63) y_pos = 63;

        OLED_DrawPoint(127 - x, y_pos, 1);
    }
}

/* ─── 主界面显示 (当前温度模式) ─── */
static void DisplayCurrentMode(void)
{
    float mcu_temp;

    OLED_Clear(0);

    /* 第1行: 标题 */
    OLED_ShowString(0, 0, "==Weather Station==", 12);

    /* 第2-3行: DS18B20 外部温度 (大字) */
    OLED_ShowString(0, 2, "Out: ", 16);
    if (g_temp_current > -100.0f) {
        OLED_ShowFloat(40, 2, g_temp_current, 3, 1, 16);
        // 度的符号用小o代替
        OLED_ShowChar(96, 2, 'C', 16);
    } else {
        OLED_ShowString(40, 2, "No Sensor!", 16);
    }

    /* 第4行: MCU 内部温度 */
    OLED_ShowString(0, 4, "MCU: ", 12);
    mcu_temp = GetMCUTemperature();
    OLED_ShowFloat(30, 4, mcu_temp, 3, 1, 12);
    OLED_ShowChar(66, 4, 'C', 12);

    /* 第5行: 心跳计数 (证明程序在跑) */
    OLED_ShowString(0, 5, "Tick:", 12);
    OLED_ShowNum(30, 5, g_tick, 6, 12);

    /* 第6-7行: 操作提示 */
    OLED_ShowString(0, 6, "KEY0:Max/Min", 12);
    OLED_ShowString(0, 7, "KEY1:Graph", 12);

    OLED_Refresh();
}

/* ─── 显示最高/最低记录 ─── */
static void DisplayMaxMinMode(void)
{
    OLED_Clear(0);

    OLED_ShowString(0, 0, "==Records==", 16);

    OLED_ShowString(0, 2, "MAX:", 12);
    OLED_ShowFloat(30, 2, g_temp_max, 3, 1, 12);
    OLED_ShowChar(66, 2, 'C', 12);

    OLED_ShowString(0, 3, "MIN:", 12);
    OLED_ShowFloat(30, 3, g_temp_min, 3, 1, 12);
    OLED_ShowChar(66, 3, 'C', 12);

    OLED_ShowString(0, 4, "CUR:", 12);
    OLED_ShowFloat(30, 4, g_temp_current, 3, 1, 12);
    OLED_ShowChar(66, 4, 'C', 12);

    OLED_ShowString(0, 6, "WK_UP:Clear", 12);
    OLED_ShowString(0, 7, "KEY0:Back", 12);

    OLED_Refresh();
}

/* ─── 显示温度曲线 ─── */
static void DisplayGraphMode(void)
{
    OLED_Clear(0);

    OLED_ShowString(0, 0, "Trend (0-50C)", 12);

    DrawTrendGraph();

    OLED_ShowString(0, 7, "KEY0:Back", 12);

    OLED_Refresh();
}

/* ─── 处理按键 ─── */
static void HandleKeys(void)
{
    uint8_t key = KEY_Scan(0);  // 不支持连按

    switch (key) {
    case KEY0_VAL:
        if (g_display_mode == MODE_CURRENT) {
            g_display_mode = MODE_MAX_MIN;
        } else {
            g_display_mode = MODE_CURRENT;  // 返回主界面
        }
        break;

    case KEY1_VAL:
        if (g_display_mode == MODE_CURRENT) {
            g_display_mode = MODE_HISTORY;
        } else {
            g_display_mode = MODE_CURRENT;
        }
        break;

    case WKUP_VAL:
        if (g_display_mode == MODE_MAX_MIN) {
            /* 清除历史记录 */
            g_temp_max = -40.0f;
            g_temp_min = 125.0f;
            SaveRecords();
        }
        break;

    default:
        break;
    }
}

/* ─── 更新温度并检查极值 ─── */
static void UpdateTemperature(void)
{
    float temp;

    /* 读 DS18B20 */
    temp = DS18B20_GetTemperature();

    /* 检查合法性 (DS18B20 范围 -55~+125°C) */
    if (temp > -55.0f && temp < 125.0f) {
        g_temp_current = temp;

        /* 更新极值记录 */
        if (temp > g_temp_max) g_temp_max = temp;
        if (temp < g_temp_min) g_temp_min = temp;

        /* 保存到 EEPROM (每10次更新保存一次, 减少写入磨损) */
        if (g_tick % 10 == 0) {
            SaveRecords();
        }

        /* 更新历史 */
        UpdateHistory(temp);
    }
}

/* ─── 串口数据上报 ─── */
static void ReportToSerial(void)
{
    float mcu_temp = GetMCUTemperature();

    printf("\r\n═══════════════════════════\r\n");
    printf("  MiniSTM32 Weather Station\r\n");
    printf("═══════════════════════════\r\n");
    printf("  External Temp: %.1f °C\r\n", g_temp_current);
    printf("  MCU Temp:      %.1f °C\r\n", mcu_temp);
    printf("  Max Record:    %.1f °C\r\n", g_temp_max);
    printf("  Min Record:    %.1f °C\r\n", g_temp_min);
    printf("  Tick:          %lu\r\n", g_tick);
    printf("═══════════════════════════\r\n");
}

/* ═══════════════════════════════════════════════════════════
 *  主函数
 * ═══════════════════════════════════════════════════════════ */
int main(void)
{
    /* ── 第1步: 系统初始化 ── */
    SystemClock_Config();
    Delay_Init();
    LED_Init();
    KEY_Init();

    /* ── 第2步: 通信/外设初始化 ── */
    USART1_Init(115200);
    AT24C02_Init();   // 初始化 I2C1 (OLED 和 EEPROM 共用)
    OLED_Init();
    ADC1_Init();

    printf("\r\nMiniSTM32 Weather Station Starting...\r\n");

    /* ── 第3步: 检测 DS18B20 ── */
    if (DS18B20_Init() == 0) {
        printf("[OK] DS18B20 detected!\r\n");
    } else {
        printf("[WARN] DS18B20 not found! Check wiring.\r\n");
    }

    /* ── 第4步: 加载历史极值 ── */
    LoadRecords();
    printf("Max: %.1f C, Min: %.1f C\r\n", g_temp_max, g_temp_min);

    /* ── 第5步: 开机画面 ── */
    OLED_ShowBootScreen();

    printf("[OK] System ready.\r\n");

    /* ═══════════════════════════════════════════════════════
     *  主循环
     * ═══════════════════════════════════════════════════════ */
    while (1) {
        /* ── 按键扫描 ── */
        HandleKeys();

        /* ── 每2秒更新一次温度 ── */
        static uint32_t last_temp_update = 0;
        if (g_tick - last_temp_update >= 2) {
            last_temp_update = g_tick;

            /* 读取温度 */
            UpdateTemperature();

            /* LED1 闪烁 (绿色 = 温度读取正常) */
            LED1_TOGGLE();

            /* 串口上报 (每10次即20秒上报一次) */
            if (g_tick % 10 == 0) {
                ReportToSerial();
            }
        }

        /* ── 显示更新 ── */
        switch (g_display_mode) {
        case MODE_CURRENT:
            DisplayCurrentMode();
            break;
        case MODE_MAX_MIN:
            DisplayMaxMinMode();
            break;
        case MODE_HISTORY:
            DisplayGraphMode();
            break;
        default:
            break;
        }

        /* ── 心跳 (1秒) ── */
        Delay_ms(1000);
        g_tick++;

        /* LED0 慢闪 = 系统正常工作 */
        if (g_tick % 2 == 0) LED0_ON(); else LED0_OFF();
    }
}
