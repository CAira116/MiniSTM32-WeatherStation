#include "gpio.h"
#include "rcc.h"
#include "spi.h"


void spi_init(void){
RCC->APB2ENR |= (1<<12);
gpio_init(GPIOA,5,AF_PP);
gpio_init(GPIOA,6,IN_FLOAT);
gpio_init(GPIOA,7,AF_PP);
gpio_init(GPIOA,4,OUT_PP);
SPI1->CR1 = (2 << 3) | (1 << 2) | (1 << 6) | (1 << 8) | (1 << 9);	
 spi_cs_high();
}

 uint8_t spi_transfer(uint8_t data){
 SPI1->DR=data;
	 while(!(SPI1->SR &(1<<1)));
	 while(!(SPI1->SR &(1<<0)));
	 return SPI1->DR;
 }
 
  void spi_cs_low(void)  { gpio_write(GPIOA, 4, 0); }
  void spi_cs_high(void) { gpio_write(GPIOA, 4, 1); }