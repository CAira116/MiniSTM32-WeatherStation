 #ifndef __I2C_H
  #define __I2C_H
  #include "stm32f10x.h"

  void    i2c_init(void);
  uint8_t i2c_send_byte(I2C_TypeDef *i2c, uint8_t addr,
  uint8_t data);
  uint8_t i2c_read_bytes(I2C_TypeDef *i2c, uint8_t addr,
  uint8_t len, uint8_t *buf);
  uint8_t i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg, uint8_t data);
  #endif