#include "stm32f10x.h"
unsigned char LedSeg[11]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,0x40};
unsigned char LedBit[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
unsigned char LedBuf[8]={10, 10, 10, 10, 10, 10, 10, 10};

void Gpio_Init (void){
	RCC->APB2ENR|=1<<0|1<<2|1<<3;
  GPIOB->CRL=0x88883333;
	GPIOA->CRH=0x33333333;
	GPIOA->CRL=0x33333333;
	GPIOB->ODR |= 0x00F0;   // PB4~PB7 上拉
	AFIO->MAPR=(AFIO->MAPR&~(7<<24))|4<<24;
	
}
void Delay(unsigned long int n)
{
    unsigned long int i;
    for(i= 0;i< n;i++)
    {
        ;//空语句
    }
}

unsigned char KeyScan(void)
{
    //首行输出0，其他行输出1
    GPIOB->ODR = (GPIOB->ODR & 0xFFF0)| 0x000E;
    Delay(200);
    if((GPIOB->IDR & 1<<4)== 0)         return 0x10;
    else if((GPIOB->IDR & 1<<5)== 0)    return 0x11;
    else if((GPIOB->IDR & 1<<6)== 0)    return 0x12;
    else if((GPIOB->IDR & 1<<7)== 0)    return 0x13;
    
    //row1输出0，其他行输出1
    GPIOB->ODR = (GPIOB->ODR & 0xFFF0)| 0x000D;
    Delay(200);
    if((GPIOB->IDR & 1<<4)== 0)         return 0x14;
    else if((GPIOB->IDR & 1<<5)== 0)    return 0x15;
    else if((GPIOB->IDR & 1<<6)== 0)    return 0x16;
    else if((GPIOB->IDR & 1<<7)== 0)    return 0x17;
    
    //row2输出0，其他行输出1
    GPIOB->ODR = (GPIOB->ODR & 0xFFF0)| 0x000B;
    Delay(200);
    if((GPIOB->IDR & 1<<4)== 0)         return 0x18;
    else if((GPIOB->IDR & 1<<5)== 0)    return 0x19;
    else if((GPIOB->IDR & 1<<6)== 0)    return 0x1A;
    else if((GPIOB->IDR & 1<<7)== 0)    return 0x1B;
    
    //row3输出0，其他行输出1
    GPIOB->ODR = (GPIOB->ODR & 0xFFF0)| 0x0007;
    Delay(200);
    if((GPIOB->IDR & 1<<4)== 0)         return 0x1C;
    else if((GPIOB->IDR & 1<<5)== 0)    return 0x1D;
    else if((GPIOB->IDR & 1<<6)== 0)    return 0x1E;
    else if((GPIOB->IDR & 1<<7)== 0)    return 0x1F;
    
    return 0x00;
}
int main(void){
Gpio_Init();
	int i=0;
	int cnt=0;
	int key,pre_key=0x00;
	
  
	while(1){
	key= KeyScan();
	if(pre_key==0x00&&key!=0){
	key=KeyScan();
	if(key>=0x10&&key<=0x19)
	{
		if(cnt<8){
	LedBuf[cnt]=key-0x10;
		cnt++;}
	}	
	
  else if (key == 0x1A)   // 退格
  {
  if (cnt > 0) {
      cnt--;
      LedBuf[cnt] = 10;   // 把刚退的那位恢复成横杠
  }
}
//	 else if (key == 0x1A)   // 功能键1: 退格
//  {if(cnt>0)
//      cnt--;
//  }
  else if (key == 0x1D)   // 功能键2: 清零
  {
		for (int j = 0; j < 8; j++)
      LedBuf[j] = 10;
      cnt = 0;
  }
	} pre_key = key; 
	
for (i = 0; i < 8; i++) {
	GPIOA->ODR=(GPIOA->ODR&0xFF00)|LedBit[i];
	GPIOA->ODR=(GPIOA->ODR&0x00FF)|LedSeg[LedBuf[i]]<<8;
      Delay(500);
	
  
}
	}

}
