 #ifndef __SPI_H
 #define __SPI_H
 #include "stm32f10x.h"
 
 void spi_init(void);
 uint8_t spi_transfer(uint8_t data);
 void spi_cs_low(void);
 void spi_cs_high(void);
 
 #endif