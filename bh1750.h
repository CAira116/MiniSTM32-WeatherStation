 #ifndef __BH1750_H
  #define __BH1750_H
  #include "stm32f10x.h"
	
	void bh1750_init(void);
	float bh1750_read_lux(void);
	
	
	#endif