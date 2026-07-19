#include "systick.h"
#include "FreeRTOS.h"
#include "task.h"

static void dwt_init(void) {
      if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
          CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
          DWT->CYCCNT = 0;
          DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
      }
  }
 
  uint32_t systick_get_ms(void) {
      if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
          return xTaskGetTickCount();    // 调度器跑起来了 →用 FreeRTOS 的心跳
      }
      dwt_init();
      return DWT->CYCCNT / 72000;        // 调度器没跑 → 用DWT 自己算
  }
	 
 void systick_delay_ms(uint32_t ms) {
      if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
  ms >= 10) {
          vTaskDelay(pdMS_TO_TICKS(ms));   // 让出 CPU
      } else {
          dwt_init();
          uint32_t start = DWT->CYCCNT;
          uint32_t end = start + ms * 72000;
          while (DWT->CYCCNT < end);        // 硬等
      }
  }
void systick_delay_us(uint32_t us) {
      dwt_init();
      uint32_t start = DWT->CYCCNT;
      uint32_t end = start + us * 72;   // 72 周期 = 1us@72MHz
      while (DWT->CYCCNT < end);
  }

 
 
  
	