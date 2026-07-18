#include "esp8266.h"
#include "systick.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>



// 前提：usart2_init() 和 systick_clock_init() 必须已调用，否则死等/死循环
  // 返回：1=等到 keyword   0=超时
int esp_cmd(const char *cmd,const char *keyword,uint32_t timeout_ms){
  uint32_t start;
	uint8_t b;
	int progress =0;
	while(usart_try_receive(USART2,&b));
	usart_send_string(USART2,cmd);//把cmd发给USART2
  start=systick_get_ms();//记录发送时间
 
	while(1){
		if((systick_get_ms()-start)>=timeout_ms)
		return 0;
		if(usart_try_receive(USART2,&b)){
		if(b==keyword[progress]){
			progress++;
			if(keyword[progress]=='\0')
			return 1;
		}
		else{
		progress=0;
			if(b==keyword[0]){
			progress=1;
			}
		}
		}
	}
}

 int esp8266_connect_wifi(void)  {
 
	 if (!esp_cmd("AT\r\n", "OK", 1000)) return 0;
	
	 if (!esp_cmd("AT+CWMODE=1\r\n","OK",1000)) return 0;

	if (!esp_cmd("AT+CWJAP=\"0\",\"88888888\"\r\n","GOT IP",15000)) return 0;
	 return 1;
 }
 
 int esp8266_tcp_connect(void){
    // 挂旧连接先断掉，否则重复测会 CIPSTART ERROR
    esp_cmd("AT+CIPCLOSE\r\n", "OK", 2000);

    usart_send_string(USART1, "TCP: CIPSTART...\r\n");
    if(!esp_cmd("AT+CIPSTART=\"TCP\",\"10.247.187.59\",8899\r\n","OK",5000)) {
        usart_send_string(USART1, "TCP: CIPSTART FAIL\r\n");
        return 0;
    }
    return 1;
 }
 
 int esp8266_tcp_send(const char*data){
 char buf[32];
	sprintf(buf,"AT+CIPSEND=%u\r\n",strlen(data));
	  usart_send_string(USART1, "TCP: CIPSEND...\r\n");
	 if(!esp_cmd(buf,">",2000)){
	 usart_send_string(USART1, "TCP: CIPSEND FAIL\r\n");
        return 0;
	 }
	  usart_send_string(USART1, "TCP: SEND data...\r\n");
	 if(!esp_cmd(data,"SEND OK",3000)) {
        usart_send_string(USART1, "TCP: SEND FAIL\r\n");
        return 0;
    }
	 return 1;
 }