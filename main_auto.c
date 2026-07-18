#include "usart.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include "esp8266.h"
#include "i2c.h"
#include "bmp280.h"
#include "bh1750.h"
#include "dht11.h"
#include <stdio.h>

	
 int main(){
      // ===== ① 初始化区 =====
     
	 rcc_sysclk_init();
	 usart_init(USART1,115200);
	 usart2_init(115200);
	 systick_clock_init();// 现有的 rcc/usart/usart2/systick 保留
      i2c_init();
	 bmp280_init();// 新增:i2c_init() + bmp280_init()  ← 顺序:先 i2c 后bmp280,谁是谁的地基?
     bh1750_init();
  dht11_init(GPIOA, 0);
	 
      // ===== ② 一次性区 =====
     if(!esp8266_connect_wifi()){
      usart_send_string(USART1,"WIFI FAIL\r\n");
      while(1);   // 程序到此为止,原地罚站
  }// WIFI 连接(现有,保留)
      //   失败 → 喊话 + while(1); 卡住别往下走
      if(!esp8266_tcp_connect()){// TCP 连接(改名后的 tcp_connect)
     usart_send_string(USART1,"TCP FAIL\r\n");
				while(1);//   失败 → 同上
			}
      // ===== ③ 常驻区 while(1) =====
      while(1){
			 float temp;
	     float press;
			 float lux;  
			 char msg[64];
				uint8_t dht_buf[5];
        float hum = 0.0f;
        if(dht11_read(GPIOA, 0, dht_buf))
        hum = (float)dht_buf[0];
			  lux   = bh1750_read_lux();
				temp=bmp280_read_temp();
				press=bmp280_read_press();
				sprintf(msg, "T=%.1fC,H=%.0f%%,P=%.1fhPa,L=%.0flux\r\n", temp,  hum, press, lux);
				if(!esp8266_tcp_send(msg)){
				 usart_send_string(USART1,"REPORT FAIL\r\n");
				}
				else{
				usart_send_string(USART1,"REPORT OK\r\n");
				}
				systick_delay_ms(30000);
			}// 读温度、读气压(两个 float 变量接住)
      // 拼报文到 char msg[64]
      // tcp_send(msg),门卫式:成功喊 REPORT OK,失败喊 REPORTFAIL
      // 等 5 秒(测试期用 5s,上板验过再改 30s)
  }