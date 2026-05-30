/**
 * DS18B20 数字温度传感器驱动 (OneWire 单总线协议)
 *
 * ─────────────────────────────────────────────
 *  半导体知识链接：
 *  DS18B20 内部含有一个 bandgap 温度传感器 + Σ-Δ ADC
 *  温度变化 → 两个振荡器频率差 → 计数器 → 数字量
 *  这就是课上学的 "温度传感器 + ADC" 在芯片里的实际实现
 * ─────────────────────────────────────────────
 *
 * 时序要求（关键！）:
 *   写 "1": 拉低 1-15µs → 释放 → 等待 60µs
 *   写 "0": 拉低 60µs → 释放 → 等待 2µs
 *   读:     拉低 1µs → 释放 → 5µs 内采样 → 等待 60µs
 *
 * 使用 DWT 实现精确定时，不占用定时器资源
 */
#include "ds18b20.h"
#include "delay.h"

/**
 * 初始化 DS18B20
 * 发送复位脉冲 → 等待存在脉冲
 * @return 0=检测到设备, 1=未检测到
 *
 * 时序: 主机拉低 480µs+ → 释放 → DS18B20 拉低 60-240µs 作为应答
 */
uint8_t DS18B20_Init(void)
{
    uint8_t presence = 0;

    // 1. 使能 GPIO 时钟
    RCC_APB2PeriphClockCmd(DS18B20_RCC, ENABLE);

    // 2. 主机发送复位脉冲: 拉低 >= 480µs
    DS18B20_OUT_LO();
    Delay_us(500);   // 拉低 500µs
    DS18B20_IN_FLOAT();  // 释放总线
    Delay_us(60);    // 等待 60µs

    // 3. 检测存在脉冲: DS18B20 会在 60-240µs 内拉低总线
    presence = (DS18B20_READ() == 0);

    // 4. 等待完整时隙结束
    Delay_us(420);   // 总共约 960µs 的复位时序

    return presence;  // 0=成功检测到
}

/**
 * 写一个字节到 DS18B20 (LSB first)
 */
void DS18B20_WriteByte(uint8_t dat)
{
    uint8_t i;

    for (i = 0; i < 8; i++) {
        // 每个写时隙 >= 60µs, 两个时隙间 >= 1µs 恢复
        DS18B20_OUT_LO();  // 拉低开始写时隙
        Delay_us(2);       // 维持 2µs

        if (dat & 0x01) {
            // 写 "1": 尽早释放总线
            DS18B20_IN_FLOAT();
            Delay_us(60);  // 等待时隙结束
        } else {
            // 写 "0": 保持拉低 60µs
            Delay_us(60);
            DS18B20_IN_FLOAT();
        }

        Delay_us(2);  // 时隙间恢复
        dat >>= 1;
    }
}

/**
 * 从 DS18B20 读一个字节 (LSB first)
 */
uint8_t DS18B20_ReadByte(void)
{
    uint8_t i, dat = 0;

    for (i = 0; i < 8; i++) {
        dat >>= 1;

        // 读时隙: 主机拉低 1-2µs 后释放
        DS18B20_OUT_LO();
        Delay_us(2);
        DS18B20_IN_FLOAT();
        Delay_us(6);  // 等待信号稳定, 在 15µs 窗口内采样

        if (DS18B20_READ()) {
            dat |= 0x80;  // MSB first 的另一种实现
        }

        Delay_us(55);  // 等待时隙结束
    }

    return dat;
}

/**
 * 获取温度值
 * 流程: Reset → Skip ROM (0xCC) → Convert T (0x44) → 等待转换 →
 *       Reset → Skip ROM (0xCC) → Read Scratchpad (0xBE) → 读9字节
 *
 * @return 温度值 (°C), 精度 0.0625°C (12-bit 模式)
 *
 * 暂存器字节布局:
 *   Byte 0: 温度 LSB
 *   Byte 1: 温度 MSB  (2's complement)
 *   Byte 2: TH 寄存器
 *   Byte 3: TL 寄存器
 *   Byte 4: 配置寄存器 (bit6-5: 分辨率 00=9bit 01=10bit 10=11bit 11=12bit)
 *   Byte 5-7: 保留
 *   Byte 8: CRC
 */
float DS18B20_GetTemperature(void)
{
    uint8_t buf[9];
    int16_t raw;
    float temp;

    // --- 启动转换 ---
    DS18B20_Init();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);  // 跳过 ROM (单设备)
    DS18B20_WriteByte(DS18B20_CMD_CONVERT_T); // 开始转换

    // 12-bit 分辨率最大转换时间 750ms, 这里等 800ms 最安全
    Delay_ms(800);

    // --- 读取结果 ---
    DS18B20_Init();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCH);

    // 读取 9 字节暂存器
    for (uint8_t i = 0; i < 9; i++) {
        buf[i] = DS18B20_ReadByte();
    }

    // --- 简单 CRC 校验 (可选) ---
    // 此处省略, 如需严谨请补充 CRC-8 校验

    // --- 温度换算 ---
    raw = (int16_t)(buf[1] << 8) | buf[0];

    // 12-bit 分辨率: 每个 LSB = 0.0625°C
    temp = raw * 0.0625f;

    return temp;
}
