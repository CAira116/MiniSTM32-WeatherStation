#include "gpio.h"
#include "systick.h"
#include "dht11.h"

void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us * 12; i++) {
        __NOP();  
    }
}

uint8_t dht11_read(GPIO_TypeDef *port, uint8_t pin,uint8_t *buf) {
      gpio_init(port, pin, OUT_PP);   // 切回输出模式
      gpio_write(port, pin, 1);
      systick_delay_ms(1000);         // DHT11 上电后需 1 秒稳定（上一次读完是输入）
      gpio_write(port, pin, 0);
		systick_delay_ms(18);
		gpio_write(port,pin,1);
		delay_us(40);
     
 gpio_init(port, pin, IN_PU);
      uint32_t t0 = systick_get_ms();
  while(gpio_read(port,pin)==0)
      if(systick_get_ms() - t0 > 10) return 0;
  t0 = systick_get_ms();
  while(gpio_read(port,pin)==1)
      if(systick_get_ms() - t0 > 10) return 0;
	
     int i;
		 for (int j = 0; j < 5; j++) buf[j] = 0; 
		for(i=0;i<40;i++){
		t0 = systick_get_ms();
  while(gpio_read(port,pin)==0)
      if(systick_get_ms() - t0 > 10) return 0;
			
			delay_us(30);
			if(gpio_read(port,pin)==1){
			  buf[i / 8] = (buf[i / 8] << 1) | 1;
			}
			else{
				 buf[i / 8] = buf[i / 8] << 1; 
			}
			 t0 = systick_get_ms();
  while(gpio_read(port,pin)==1)
      if(systick_get_ms() - t0 > 10) return 0;
		}
       if (buf[4] == (uint8_t)(buf[0] + buf[1] + buf[2] +buf[3]))
      return 1;
  else
      return 0;

      
  }
	
 void dht11_init(GPIO_TypeDef *port, uint8_t pin) {
      gpio_init(port, pin, OUT_PP);
      gpio_write(port, pin, 1);
  }