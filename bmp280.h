 #ifndef __BMP280_H
  #define __BMP280_H
  #include "stm32f10x.h"
	
	 void  bmp280_init(void);          // 读校准数据，存到 static 变量
  float bmp280_read_temp(void);     // 返回 °C
  float bmp280_read_press(void);    // 返回 Pa
	
	#endif