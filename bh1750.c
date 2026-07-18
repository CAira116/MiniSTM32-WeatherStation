#include "systick.h"
#include "bh1750.h"
#include "i2c.h"
#include "stm32f10x.h"

void bh1750_init(void){
  i2c_send_byte(I2C1,0x23,0x10);
  systick_delay_ms(180);
}
	
float bh1750_read_lux(void){

    uint8_t buf[2] = {0};
    i2c_read_bytes(I2C1, 0x23, 2, buf);
			
    uint16_t raw = (buf[0] << 8) | buf[1];  
    float lux = raw / 1.2;   
    return lux;		
}
