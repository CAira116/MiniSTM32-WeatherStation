/**
 * AT24C02 EEPROM 驱动 (I2C 接口, 256 字节)
 *
 * ─────────────────────────────────────────────
 *  半导体知识链接：
 *  EEPROM 存储单元基于浮栅晶体管 (Floating-Gate MOSFET)
 *  Fowler-Nordheim 隧穿注入电子 → 阈值电压偏移 → "0"/"1"
 *  和你半导体物理课学的 MOS 结构直接相关
 * ─────────────────────────────────────────────
 *
 * 本气象站用途: 存储最高/最低温度记录
 * 地址分配:
 *   0x00-0x03: 最高温度 (float, 4 字节)
 *   0x04-0x07: 最低温度 (float, 4 字节)
 *   0x08-0x0F: 保留
 */
#include "24c02.h"
#include "delay.h"

/* I2C1 引脚: PB6(SCL), PB7(SDA) */
#define AT24C02_I2C         I2C1
#define AT24C02_I2C_RCC     RCC_APB1Periph_I2C1
#define AT24C02_GPIO_RCC    RCC_APB2Periph_GPIOB
#define AT24C02_GPIO        GPIOB
#define AT24C02_SCL_PIN     GPIO_Pin_6
#define AT24C02_SDA_PIN     GPIO_Pin_7

/**
 * I2C1 初始化 (100kHz 标准模式)
 */
void AT24C02_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    I2C_InitTypeDef   I2C_InitStructure;

    // 开启时钟
    RCC_APB2PeriphClockCmd(AT24C02_GPIO_RCC, ENABLE);
    RCC_APB1PeriphClockCmd(AT24C02_I2C_RCC,  ENABLE);

    // PB6(SCL), PB7(SDA) → 复用开漏输出
    GPIO_InitStructure.GPIO_Pin   = AT24C02_SCL_PIN | AT24C02_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_OD;   // 复用开漏 (关键!)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(AT24C02_GPIO, &GPIO_InitStructure);

    // I2C1 配置
    I2C_InitStructure.I2C_Mode        = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle   = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;          // 主机模式, 不需要自身地址
    I2C_InitStructure.I2C_Ack         = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed  = 100000;        // 100kHz

    I2C_Init(AT24C02_I2C, &I2C_InitStructure);
    I2C_Cmd(AT24C02_I2C, ENABLE);
}

/**
 * 写一个字节到指定地址
 * 注意: 24C02 写入后需要 ~5ms 等待内部写入完成
 */
void AT24C02_WriteByte(uint8_t addr, uint8_t dat)
{
    // 等待总线空闲
    while (I2C_GetFlagStatus(AT24C02_I2C, I2C_FLAG_BUSY));

    // 产生起始信号
    I2C_GenerateSTART(AT24C02_I2C, ENABLE);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    // 发送设备地址 (写)
    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDR_WRITE, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    // 发送内存地址
    I2C_SendData(AT24C02_I2C, addr);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // 发送数据
    I2C_SendData(AT24C02_I2C, dat);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // 停止信号
    I2C_GenerateSTOP(AT24C02_I2C, ENABLE);

    // 等待内部写入完成 (EEPROM 写入周期)
    Delay_ms(5);
}

/**
 * 从指定地址读一个字节
 */
uint8_t AT24C02_ReadByte(uint8_t addr)
{
    uint8_t dat = 0;

    // 等待总线空闲
    while (I2C_GetFlagStatus(AT24C02_I2C, I2C_FLAG_BUSY));

    // --- 第1步: 假写, 发送要读的地址 ---
    I2C_GenerateSTART(AT24C02_I2C, ENABLE);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDR_WRITE, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(AT24C02_I2C, addr);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // --- 第2步: 重新起始, 读数据 ---
    I2C_GenerateSTART(AT24C02_I2C, ENABLE);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDR_READ, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    // NACK + STOP 结束接收 (单字节)
    I2C_AcknowledgeConfig(AT24C02_I2C, DISABLE);
    I2C_GenerateSTOP(AT24C02_I2C, ENABLE);

    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED));
    dat = I2C_ReceiveData(AT24C02_I2C);

    I2C_AcknowledgeConfig(AT24C02_I2C, ENABLE);  // 恢复 ACK

    return dat;
}

/**
 * 连续写 (支持页内, 跨页需自行处理)
 */
void AT24C02_WriteBytes(uint8_t addr, uint8_t *buf, uint8_t len)
{
    while (I2C_GetFlagStatus(AT24C02_I2C, I2C_FLAG_BUSY));

    I2C_GenerateSTART(AT24C02_I2C, ENABLE);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDR_WRITE, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(AT24C02_I2C, addr);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    for (uint8_t i = 0; i < len; i++) {
        I2C_SendData(AT24C02_I2C, buf[i]);
        while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }

    I2C_GenerateSTOP(AT24C02_I2C, ENABLE);
    Delay_ms(5);
}

/**
 * 连续读
 */
void AT24C02_ReadBytes(uint8_t addr, uint8_t *buf, uint8_t len)
{
    while (I2C_GetFlagStatus(AT24C02_I2C, I2C_FLAG_BUSY));

    // 假写地址
    I2C_GenerateSTART(AT24C02_I2C, ENABLE);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDR_WRITE, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(AT24C02_I2C, addr);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // 重新起始, 读
    I2C_GenerateSTART(AT24C02_I2C, ENABLE);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDR_READ, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for (uint8_t i = 0; i < len; i++) {
        if (i == len - 1) {
            I2C_AcknowledgeConfig(AT24C02_I2C, DISABLE);  // 最后一个字节 NACK
        }
        while (!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED));
        buf[i] = I2C_ReceiveData(AT24C02_I2C);
    }

    I2C_GenerateSTOP(AT24C02_I2C, ENABLE);
    I2C_AcknowledgeConfig(AT24C02_I2C, ENABLE);
}
