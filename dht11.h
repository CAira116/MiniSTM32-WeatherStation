  #ifndef __DHT11_H
  #define __DHT11_H
  #include "stm32f10x.h"

  void    dht11_init(GPIO_TypeDef *port, uint8_t pin);
  uint8_t dht11_read(GPIO_TypeDef *port, uint8_t pin,
  uint8_t *buf);

  #endif