/**
 * 按键驱动
 * KEY0  = PE4, 按下为低电平 (GND)
 * KEY1  = PE3, 按下为低电平 (GND)
 * WK_UP = PA0, 按下为高电平 (3.3V)
 *
 * 消抖原理：检测到按下 → 延时10ms → 再次确认 → 等待释放（mode=1连按除外）
 */
#include "key.h"
#include "delay.h"

void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOA, ENABLE);

    // KEY0, KEY1: 下拉输入（平时低，按下检测到高）
    // 实际上板子用的是上拉 → 平时高，按下低
    GPIO_InitStructure.GPIO_Pin   = KEY0_PIN | KEY1_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;  // 上拉输入
    GPIO_Init(KEY0_PORT, &GPIO_InitStructure);

    // WK_UP: 下拉输入（平时低，按下高）
    GPIO_InitStructure.GPIO_Pin   = WKUP_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;  // 下拉输入
    GPIO_Init(WKUP_PORT, &GPIO_InitStructure);
}

/**
 * 按键扫描
 * @param mode: 0 = 不支持连按（松开才再次生效）
 *              1 = 支持连按（按住持续返回键值）
 * @return 键值
 */
uint8_t KEY_Scan(uint8_t mode)
{
    static uint8_t key_up_flag = 1;  // 按键松开标志

    if (mode) key_up_flag = 1;  // 连按模式下每次都能触发

    if (key_up_flag && (KEY0_PRESS || KEY1_PRESS || WKUP_PRESS)) {
        Delay_ms(10);  // 消抖

        key_up_flag = 0;

        if (KEY0_PRESS)  return KEY0_VAL;
        if (KEY1_PRESS)  return KEY1_VAL;
        if (WKUP_PRESS)  return WKUP_VAL;
    }
    else if (!KEY0_PRESS && !KEY1_PRESS && !WKUP_PRESS) {
        key_up_flag = 1;  // 所有按键都松开了
    }

    return KEY_NONE;
}
