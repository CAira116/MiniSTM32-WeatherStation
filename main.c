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
      rcc_sysclk_init();
	 usart_init(USART1,115200);
	 usart2_init(115200);
	 systick_clock_init();
	 i2c_init();
	 bmp280_init();
	 bh1750_init();
	 dht11_init(GPIOA, 0);

      // ===== 一次性:WiFi+TCP连接 =====
     if(!esp8266_connect_wifi()){
      usart_send_string(USART1,"WIFI FAIL\r\n");
      while(1);
     }
     if(!esp8266_tcp_connect()){
      usart_send_string(USART1,"TCP FAIL\r\n");
      while(1);
     }

      // ===== 常驻:每30秒自动上报 =====
      while(1){
          static int count = 0;
          float temp;
          float press;
          float lux;
          char msg[64];
          int uptime = (int)(systick_get_ms() / 1000);
          uint8_t dht_buf[5];
          float hum = 0.0f;
          if(dht11_read(GPIOA, 0, dht_buf))
              hum = (float)dht_buf[0];
          lux   = bh1750_read_lux();
          temp  = bmp280_read_temp();
          press = bmp280_read_press();
          sprintf(msg, "[#%d,up=%us] T=%.1fC,H=%.0f%%,P=%.1fhPa,L=%.0flux\r\n",
                  ++count, uptime, temp, hum, press, lux);
          if(esp8266_tcp_send(msg)){
              usart_send_string(USART1,"REPORT OK\r\n");
          }
          else{
              usart_send_string(USART1, "LINK LOST,reconnecting...\r\n");
              if (esp8266_connect_wifi() && esp8266_tcp_connect()){
                  usart_send_string(USART1, "RECONNECT OK\r\n");
                  if (esp8266_tcp_send(msg))
                      usart_send_string(USART1,"RETRY REPORT OK\r\n");
                  else
                      usart_send_string(USART1,"RETRY REPORT FAIL\r\n");
              }
              else{
                  usart_send_string(USART1, "RECONNECT FAIL\r\n");
              }
          }
          systick_delay_ms(30000);
      }
 }
