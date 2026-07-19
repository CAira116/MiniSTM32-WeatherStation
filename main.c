/* ================================================================
 *  MiniSTM32-WeatherStation — FreeRTOS 版
 *  学习日期: 2026-07-19
 *
 *  【今日学习路线】
 *  Task → Queue → Software Timer → Mutex → Binary Semaphore
 *  → Task Notification
 *
 *  【架构: 2 任务 + 1 定时器回调】
 *
 *   vTimerCallback (定时器服务任务内部, pri=1)
 *    │  每 30s 触发一次
 *    │  读传感器 → 上锁(I2C) → 解锁 → 发两个队列 → 任务通知叫醒 Display
 *    │
 *    ├─[g_display_queue]──▶ Task_display (pri=3)
 *    │                       等任务通知 → 收队列 → 上锁 → 刷OLED → 解锁
 *    │
 *    └─[g_report_queue]───▶ TaskReport (pri=1)
 *                           阻塞等队列 → 发TCP → 循环
 *
 *  【全局通信对象一览】
 *   g_display_queue  — 队列: vTimerCallback → Task_display
 *   g_report_queue   — 队列: vTimerCallback → TaskReport
 *   g_i2c_mutex      — 互斥锁: 保护 I2C 总线
 *   hDisplayTask     — 任务句柄: 存 Task_display 身份证，给 xTaskNotifyGive 用
 * ================================================================ */

#include "usart.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include "esp8266.h"
#include "i2c.h"
#include "bmp280.h"
#include "bh1750.h"
#include "dht11.h"
#include "ssd1306.h"
#include "exti.h"
#include "FreeRTOS.h"
#include "task.h"   // 任务: xTaskCreate / vTaskDelay / ulTaskNotifyTake / xTaskNotifyGive
#include "queue.h"  // 队列: xQueueCreate / xQueueSend / xQueueReceive
#include "timers.h" // 软件定时器: xTimerCreate / xTimerStart
#include "semphr.h" // 信号量/互斥锁: xSemaphoreCreateMutex / Take / Give
#include <stdio.h>

/* ───────────────────────────────────────────────────────────
 * 传感器数据包结构体
 * 替代原来的 5 个零散全局变量 (g_temp/g_hum/g_press/g_lux/g_count)
 * count 不在结构体里——它是消费者的私有计数，各自用 static int 管理
 * ─────────────────────────────────────────────────────────── */
typedef struct {
    float temp;   // BMP280 温度, ℃
    float hum;    // DHT11 湿度, %
    float press;  // BMP280 气压, hPa
    float lux;    // BH1750 光照, lux
} SensorData;

/* ──── 队列句柄 ──────────────────────────────────────────────
 * 创建: xQueueCreate(队列长度, 每条大小)
 * 为什么两个队列?
 *   队列的一条数据只被一个人取走——Display 拿了 Report 就等不到
 *   两个消费者 → 各建一个队列，vTimerCallback 里 xQueueSend 两次
 * ─────────────────────────────────────────────────────────── */
QueueHandle_t g_display_queue;  // vTimerCallback → Task_display
QueueHandle_t g_report_queue;   // vTimerCallback → TaskReport

/* ──── I2C 互斥锁 (Mutex) ───────────────────────────────────
 * 创建: xSemaphoreCreateMutex() — 初始开锁状态
 * 上锁: xSemaphoreTake() — 别人在用就排队等
 * 解锁: xSemaphoreGive() — 我用完了
 * 保护: BH1750/BMP280/SSD1306 共用 I2C 总线
 *
 * Mutex 有优先级继承(防优先级反转), Take/Give 必须同任务 → 用于互斥
 * Binary Semaphore 无优先级继承, Take/Give 可不同任务 → 用于通知
 * ─────────────────────────────────────────────────────────── */
SemaphoreHandle_t g_i2c_mutex;

/* ──── Display 任务句柄: 给任务通知用 ───────────────────────
 * 赋值: xTaskCreate() 最后一个参数填 &hDisplayTask
 * 使用: vTimerCallback 里 xTaskNotifyGive(hDisplayTask) 叫醒 Display
 * =NULL: 初始为空，xTaskCreate 出错时容易发现
 * ─────────────────────────────────────────────────────────── */
TaskHandle_t hDisplayTask = NULL;

/* ───────────────────────────────────────────────────────────
 * Task_display — 优先级 3 (最高)
 *   流程: 等任务通知 → 收队列 → 锁I2C → 刷OLED → 解锁 → 循环
 *
 * 任务通知 (Task Notification):
 *   每个任务内置通信通道，不需要单独创建信号量对象
 *   比二值信号量快 ~45%
 *   ulTaskNotifyTake(pdTRUE, portMAX_DELAY):
 *     pdTRUE = 收到后清零计数器 (跟二值信号量行为一致)
 *     portMAX_DELAY = 没收到就睡着，等 vTimerCallback 发 xTaskNotifyGive
 *
 * 互斥锁范围最小化: 只包 I2C 操作
 *   xQueueReceive 和 sprintf 不碰 I2C — 放锁外
 * ─────────────────────────────────────────────────────────── */
void Task_display(void *param)
{
    char line[22];
	  static float t_max = -99, t_min = 99;   // 温度极值
    static float h_max = 0,   h_min = 100;  // 湿度极值
    static int count = 0; /* 私有: 刷新次数，别的任务碰不到 */
    static uint8_t page = 0 ;
	  static uint32_t last_btn_time = 0;
	  static SensorData last_data ={0};
    while (1) {
        SensorData data;
        int uptime = (int)(systick_get_ms() / 1000);

        /* 睡着等通知: vTimerCallback 发了 xTaskNotifyGive 才醒 */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			
			if (xQueueReceive(g_display_queue,&data,0)==pdTRUE){
			last_data=data;
			if (data.temp > t_max) t_max = data.temp;
      if (data.temp < t_min) t_min = data.temp;
      if (data.hum  > h_max) h_max = data.hum;
      if (data.hum  < h_min) h_min = data.hum;
				count ++;
			}
			else{
				uint32_t now = systick_get_ms(); 
				if(now-last_btn_time > 70){
			page=(page+1)%3;
				}
			}

        /* ── I2C 互斥锁: 保护 OLED ── */
        xSemaphoreTake(g_i2c_mutex, portMAX_DELAY);
        ssd1306_clear();
			 switch(page){
			   case 0:
					 sprintf(line, "T=%.1fC H=%.0f%%", last_data.temp, last_data.hum);
        ssd1306_show_str(0, line);
        sprintf(line, "P=%.1fhPa L=%.0flux", last_data.press, last_data.lux);
        ssd1306_show_str(1, line);
       sprintf(line, "#%d up=%ds", count, uptime);
           ssd1306_show_str(3,line);
				 break ;
				 
				 case 1:
					ssd1306_show_str(0,"==SYSTEM==");
          sprintf(line,"UP:%d",uptime);
          ssd1306_show_str(1,line);
          sprintf(line,"Dis:%d",count);
           ssd1306_show_str(2,line);
          ssd1306_show_str(3,"BTN: next page");
           break ;				 
				 case 2:
					 ssd1306_show_str(0, "=== EXTREMES ===");
      sprintf(line, "T:%.1f/%.1fC", t_max, t_min);
      ssd1306_show_str(1, line);
      sprintf(line, "H:%.0f/%.0f%%", h_max, h_min);
       ssd1306_show_str(2, line);
      sprintf(line, "#%d up=%ds", count, uptime);
      ssd1306_show_str(3, line);
      break;					 
			 }
        
        xSemaphoreGive(g_i2c_mutex); /* ── 解锁 ── */

     
    }
}

/* ───────────────────────────────────────────────────────────
 * TaskReport — 优先级 1
 *   流程: 阻塞等队列 → 拼消息 → 发 TCP → 循环
 *
 * 为什么没有 vTaskDelay?
 *   xQueueReceive(..., portMAX_DELAY) 本身就是阻塞的:
 *     没数据 → 自动睡着
 *     有数据 → 立刻醒 → 干活 → 循环回去继续等
 *   不需要额外延时
 * ─────────────────────────────────────────────────────────── */
void TaskReport(void *param)
{
    char msg[80];
    static int count = 0; /* 私有: "第几条上报" */

    while (1) {
        SensorData data;
        int uptime = (int)(systick_get_ms() / 1000);

        /* 阻塞等队列: 没新数据就睡着，省 CPU */
        xQueueReceive(g_report_queue, &data, portMAX_DELAY);

        sprintf(msg, "[up=%us] [#%d] T=%.1fC,H=%.0f%%,P=%.1fhPa,L=%.0flux\r\n",
                uptime, ++count, data.temp, data.hum, data.press, data.lux);

        if (esp8266_tcp_send(msg)) {
            usart_send_string(USART1, "REPORT OK\r\n");
        } else {
            usart_send_string(USART1, "LINK LOST,reconnecting...\r\n");
            if (esp8266_connect_wifi() && esp8266_tcp_connect()) {
                usart_send_string(USART1, "RECONNECT OK\r\n");
                if (esp8266_tcp_send(msg))
                    usart_send_string(USART1, "RETRY REPORT OK\r\n");
                else
                    usart_send_string(USART1, "RETRY REPORT FAIL\r\n");
            } else {
                usart_send_string(USART1, "RECONNECT FAIL\r\n");
            }
        }
    }
}

/* ───────────────────────────────────────────────────────────
 * 栈溢出钩子 (Stack Overflow Hook)
 *   触发: FreeRTOSConfig.h 设了 configCHECK_FOR_STACK_OVERFLOW=2,
 *   每次切任务时检查栈顶标记，破坏了就自动调这个函数
 *   用途: 串口打印溢出任务名 → 你调大对应任务的栈 → 重新编译
 *   ⚠ 你不主动调它 — FreeRTOS 检测到溢出时自动调
 * ─────────────────────────────────────────────────────────── */
void vApplicationStackOverflowHook(void *xTask, char *pcTaskName)
{
    usart_send_string(USART1, "\r\n!!! STACK OVERFLOW: ");
    usart_send_string(USART1, pcTaskName);
    usart_send_string(USART1, " !!!\r\n");
    while (1);
}

/* ───────────────────────────────────────────────────────────
 * 软件定时器回调 — 替代 Task_Sensor
 *   触发: 定时器服务任务每 30s 自动调一次，不需要自己建任务
 *   ⚠ 回调规则:
 *     - 不能 while(1), 执行完必须返回
 *     - 不能 vTaskDelay (定时器自己负责周期性)
 *     - 不能长时间阻塞 (如果 > 周期，下次触发就丢失了)
 *
 *   每次执行:
 *   1. 上锁 → 读 I2C 传感器 → 解锁
 *   2. xQueueSend → 发到两个队列
 *   3. xTaskNotifyGive → 叫醒 Task_display
 * ─────────────────────────────────────────────────────────── */
void vTimerCallback(TimerHandle_t xTimer)
{
    uint8_t dht_buf[5];
    SensorData data = {0}; /* ={0} 保底: DHT11 偶尔失败, 至少显示 0 不乱码 */

    /* ── I2C 互斥锁: 保护传感器读取 ── */
    xSemaphoreTake(g_i2c_mutex, portMAX_DELAY);

    if (dht11_read(GPIOA, 0, dht_buf))
        data.hum = (float)dht_buf[0]; /* 读成功才赋值 */
    data.lux   = bh1750_read_lux();
    data.temp  = bmp280_read_temp();
    data.press = bmp280_read_press();

    xSemaphoreGive(g_i2c_mutex); /* ── 解锁 ── */

    /* 发数据到两个队列: 各收各的，互不干扰 */
    xQueueSend(g_display_queue, &data, 0); /* timeout=0: 满了直接返回 */
    xQueueSend(g_report_queue,  &data, 0);

    /* 任务通知: 叫醒 Task_display
       Display 在 ulTaskNotifyTake() 上睡着，这行一发它立刻醒
       不需要 xSemaphoreCreateBinary() — 任务通知内置于每个任务 */
    xTaskNotifyGive(hDisplayTask);
}

int main(void)
{
    /* ── 阶段1: 硬件初始化 (调度器启动前执行) ── */
    rcc_sysclk_init();
    usart_init(USART1, 115200);
    usart2_init(115200);
    i2c_init();
	  exti_init(GPIOA, 1, EXTI_FALLING);
    bmp280_init();
    bh1750_init();
    dht11_init(GPIOA, 0);
    ssd1306_init();
    ssd1306_clear();

    if (!esp8266_connect_wifi()) {
        usart_send_string(USART1, "WIFI FAIL\r\n");
        while (1);
    }
    if (!esp8266_tcp_connect()) {
        usart_send_string(USART1, "TCP FAIL\r\n");
        while (1);
    }

    /* ── 阶段2: 创建 FreeRTOS 内核对象 ── */
    /* 队列: 长度 5, 每个元素 sizeof(SensorData) 字节 */
    g_display_queue = xQueueCreate(5, sizeof(SensorData));
    g_report_queue  = xQueueCreate(5, sizeof(SensorData));

    /* 互斥锁: 保护 I2C 总线 (3 个设备共用) */
    g_i2c_mutex = xSemaphoreCreateMutex();

    /* 软件定时器: 替代 Task_Sensor
       pdMS_TO_TICKS(30000) = 1000Hz tick 下 30000 tick = 30 秒
       pdTRUE = 自动重复 (periodic)
       回调在定时器服务任务上下文中执行 (不是中断！) */
    TimerHandle_t hSensorTimer;
    hSensorTimer = xTimerCreate(
        "sensor_timer",                 /* 调试用名字 */
        pdMS_TO_TICKS(30000),           /* 周期 30 秒 */
        pdTRUE,                         /* 自动重复 */
        NULL,                           /* Timer ID: 不用传 NULL */
        vTimerCallback                  /* 回调函数: 时间到了调这个 */
    );
    xTimerStart(hSensorTimer, 0);       /* 0 = 不额外延迟首次触发 */

    /* ── 阶段3: 创建任务 ── */
    /* &hDisplayTask: 把 Display 的身份证存进去，给 xTaskNotifyGive 用 */
    xTaskCreate(Task_display, "display", 256, NULL, 3, &hDisplayTask);
    xTaskCreate(TaskReport,  "report",  384, NULL, 1, NULL);

    /* ── 阶段4: 启动调度器 ──
       从这行开始 FreeRTOS 接管 CPU，main() 不会再往下走 */
    vTaskStartScheduler();

    while (1); /* 理论不可达 */
}
