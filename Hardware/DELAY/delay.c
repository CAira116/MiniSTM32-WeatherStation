/**
 * 延时函数 (delay functions)
 * 使用 SysTick 实现毫秒延时，DWT 实现微秒延时
 * 适用于 STM32F103, 72MHz 主频
 */
#include "delay.h"

static uint32_t fac_us = 0;  // 微秒倍乘因子

/**
 * 延时初始化
 * 必须在主时钟配置完成后调用
 */
void Delay_Init(void)
{
    // 开启 DWT (Data Watchpoint and Trace) 单元
    // DWT->CYCCNT 是一个自由运行的 32 位计数器，每时钟周期 +1
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // 使能 TRC

    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;  // 使能 CYCCNT

    DWT->CYCCNT = 0;  // 清零计数器

    // 72MHz → 1us = 72 个时钟周期
    fac_us = SystemCoreClock / 1000000;
}

/**
 * 微秒延时 (1~2^32/fac_us us)
 * 不占用定时器，不关中断
 */
void Delay_us(uint32_t nus)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = nus * fac_us;  // 需要等待的时钟周期数
    while ((DWT->CYCCNT - start) < ticks);
}

/**
 * 毫秒延时
 */
void Delay_ms(uint16_t nms)
{
    uint32_t i;
    for (i = 0; i < nms; i++) {
        Delay_us(1000);
    }
}
