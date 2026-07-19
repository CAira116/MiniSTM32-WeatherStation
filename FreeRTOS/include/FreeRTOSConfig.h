#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f10x.h"

/* ── 基础配置 ── */
#define configCPU_CLOCK_HZ                      72000000
#define configTICK_RATE_HZ                      1000        // 1ms 一次心跳
#define configMAX_PRIORITIES                    5           // 5 级优先级 (0~4)
#define configMINIMAL_STACK_SIZE                128         // 最小任务栈 (word)
#define configTOTAL_HEAP_SIZE                   8192        // 堆总大小 8KB
#define configMAX_TASK_NAME_LEN                 16          // 任务名最长 16 字符
#define configIDLE_SHOULD_YIELD                 1           // 空闲任务主动让出 CPU
#define configUSE_PREEMPTION                    1           // 抢占式调度
#define configUSE_TIME_SLICING                  1           // 同优先级时间片轮转
#define configUSE_16_BIT_TICKS                  0           // 32 位 tick 计数器
#define configUSE_TASK_NOTIFICATIONS            1           // 任务通知 (替代信号量的轻量方案)

/* ── 内核功能开关 ── */
#define configUSE_MUTEXES                       1           // 互斥锁
#define configUSE_RECURSIVE_MUTEXES             1           // 递归互斥锁
#define configUSE_COUNTING_SEMAPHORES           1           // 计数信号量
#define configUSE_TIMERS                        1           // 暂不启用软件定时器
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configCHECK_FOR_STACK_OVERFLOW          2           // 栈溢出检测方法2

/* ── 包含哪些 API ── */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTaskGetCurrentTaskHandle       1

/* ── 中断优先级 (STM32F103: 只有高 4 位有效, 0=最高, 15=最低) ── */
#define configKERNEL_INTERRUPT_PRIORITY         240         // 内核中断优先级 (15<<4)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    80          // 允许调 FreeRTOS API 的最高中断优先级 (5<<4)

/* ── 映射 FreeRTOS 异常处理到启动文件中的标准名 ── */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

/* 动态内存分配使能 */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0

/* 事件记录: 关闭 (不需要调试 Trace) */
#define configEVR_INITIALIZE                    0

 #define configTIMER_TASK_PRIORITY    1   //定时器服务任务的优先级
 #define configTIMER_QUEUE_LENGTH     5   //定时器命令队列长度
 #define configTIMER_TASK_STACK_DEPTH 128 //定时器服务任务栈大小

#endif /* FREERTOS_CONFIG_H */
