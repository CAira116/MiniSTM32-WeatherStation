#include "gpio.h"
#include "i2c.h"

void i2c_init(void) {
     RCC->APB2ENR |= (1<<3);
     RCC->APB1ENR |= (1<<21);
     gpio_init(GPIOB, 6, AF_OD);
     gpio_init(GPIOB, 7, AF_OD);
     I2C1->CR2 = 36;
     I2C1->CCR = 180;
     I2C1->TRISE = 37;
     I2C1->CR1 |= (1<<0);
}

uint8_t i2c_send_byte(I2C_TypeDef *i2c, uint8_t addr, uint8_t data) {
    i2c->CR1 |= (1<<8);
    while(!(i2c->SR1 & (1<<0)));
    i2c->DR = (addr<<1);
    while(!(i2c->SR1 & (1<<1)));
    (void)i2c->SR1;
    (void)i2c->SR2;
    while(!(i2c->SR1 & (1<<7)));
    i2c->DR = data;
    while(!(i2c->SR1 & (1<<2)));
    i2c->CR1 |= (1<<9);
    return 0;
}

uint8_t i2c_read_bytes(I2C_TypeDef *i2c, uint8_t addr, uint8_t len, uint8_t *buf) {
    i2c->CR1 |= (1<<8);
    while(!(i2c->SR1 & (1<<0)));
    i2c->DR = (addr<<1) | 1;
    while(!(i2c->SR1 & (1<<1)));
    (void)i2c->SR1;
    (void)i2c->SR2;
    if(len == 1){
        i2c->CR1 &= ~(1<<10);
        while(!(i2c->SR1 & (1<<6)));
        buf[0] = i2c->DR;
    }
    if(len >= 2){
        int i;
        i2c->CR1 |= (1<<10);
        for(i = 0; i < len; i++){
            if(i == len - 1)
                i2c->CR1 &= ~(1<<10);
            else
                i2c->CR1 |= (1<<10);
            while(!(i2c->SR1 & (1<<6)));
            buf[i] = i2c->DR;
        }
    }
    i2c->CR1 |= (1<<9);
    return 0;
}

uint8_t i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr, uint8_t reg, uint8_t data){
    i2c->CR1 |= (1<<8);
    while(!(i2c->SR1 & (1<<0)));
    i2c->DR = (addr<<1);
    while(!(i2c->SR1 & (1<<1)));
    (void)i2c->SR1;
    (void)i2c->SR2;
    while(!(i2c->SR1 & (1<<7)));
    i2c->DR = reg;
    while(!(i2c->SR1 & (1<<7)));
    i2c->DR = data;
    while(!(i2c->SR1 & (1<<2)));
    i2c->CR1 |= (1<<9);
    return 0;
}

void i2c_write_buf(I2C_TypeDef *i2c, uint8_t addr, uint8_t ctrl, uint8_t *buf, uint8_t len){
    i2c->CR1 |= (1<<8);
    while(!(i2c->SR1 & (1<<0)));
    i2c->DR = (addr << 1);
    while(!(i2c->SR1 & (1<<1)));
    (void)i2c->SR1;
    (void)i2c->SR2;
    while(!(i2c->SR1 & (1<<7)));
    i2c->DR = ctrl;
    for(uint8_t i = 0; i < len; i++){
        while(!(i2c->SR1 & (1<<7)));
        i2c->DR = buf[i];
    }
    while(!(i2c->SR1 & (1<<2)));
    i2c->CR1 |= (1<<9);
}
